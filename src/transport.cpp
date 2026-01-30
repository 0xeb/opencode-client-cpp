// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#include <opencode/transport.hpp>

#include <httplib.h>
#include <atomic>
#include <mutex>
#include <thread>

namespace opencode
{

// =============================================================================
// SSEParser Implementation
// =============================================================================

void SSEParser::feed(const std::string& data, const std::function<void(const SSEEvent&)>& on_event)
{
    buffer_ += data;

    // Process complete lines
    size_t pos = 0;
    while (true)
    {
        // Find next line break (SSE uses \n or \r\n)
        size_t line_end = buffer_.find('\n', pos);
        if (line_end == std::string::npos)
        {
            // No complete line, keep remainder in buffer
            buffer_ = buffer_.substr(pos);
            break;
        }

        // Extract line (removing \r if present)
        std::string line = buffer_.substr(pos, line_end - pos);
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        pos = line_end + 1;

        // Empty line = event dispatch
        if (line.empty())
        {
            if (!current_event_.data.empty() || !current_event_.event.empty())
            {
                // Remove trailing newline from data if present
                if (!current_event_.data.empty() && current_event_.data.back() == '\n')
                {
                    current_event_.data.pop_back();
                }
                on_event(current_event_);
                current_event_ = SSEEvent{};
            }
            continue;
        }

        // Parse field
        size_t colon = line.find(':');
        std::string field;
        std::string value;

        if (colon == 0)
        {
            // Comment line (starts with :), ignore
            continue;
        }
        else if (colon != std::string::npos)
        {
            field = line.substr(0, colon);
            // Skip space after colon if present
            size_t value_start = colon + 1;
            if (value_start < line.size() && line[value_start] == ' ')
            {
                value_start++;
            }
            value = line.substr(value_start);
        }
        else
        {
            field = line;
        }

        // Handle known fields
        if (field == "event")
        {
            current_event_.event = value;
        }
        else if (field == "data")
        {
            if (!current_event_.data.empty())
            {
                current_event_.data += '\n';
            }
            current_event_.data += value;
        }
        else if (field == "id")
        {
            current_event_.id = value;
        }
        else if (field == "retry")
        {
            try
            {
                current_event_.retry = std::stoi(value);
            }
            catch (...)
            {
                // Ignore invalid retry values
            }
        }
    }
}

void SSEParser::reset()
{
    buffer_.clear();
    current_event_ = SSEEvent{};
}

// =============================================================================
// HttpTransport Implementation
// =============================================================================

class HttpTransport::Impl
{
  public:
    Impl(const std::string& host, int port)
        : host_(host), port_(port), client_(host, port)
    {
        client_.set_connection_timeout(connection_timeout_);
        client_.set_read_timeout(read_timeout_);
    }

    Impl(const std::string& host, int port, const std::string& username, const std::string& password)
        : host_(host), port_(port), client_(host, port)
    {
        client_.set_basic_auth(username, password);
        client_.set_connection_timeout(connection_timeout_);
        client_.set_read_timeout(read_timeout_);
    }

    ~Impl()
    {
        stop_sse();
    }

    HttpResponse request(const HttpRequest& req)
    {
        HttpResponse response;

        // Build headers
        httplib::Headers headers;
        for (const auto& [key, value] : req.headers)
        {
            headers.insert({key, value});
        }

        // Add directory header if set
        if (!directory_.empty())
        {
            headers.insert({"x-opencode-directory", directory_});
        }

        // Always request JSON responses
        headers.insert({"Accept", "application/json"});

        // Set content type
        std::string content_type = req.content_type.value_or("application/json");

        httplib::Result result;
        if (req.method == "GET")
        {
            result = client_.Get(req.path, headers);
        }
        else if (req.method == "POST")
        {
            result = client_.Post(req.path, headers, req.body, content_type);
        }
        else if (req.method == "PUT")
        {
            result = client_.Put(req.path, headers, req.body, content_type);
        }
        else if (req.method == "PATCH")
        {
            result = client_.Patch(req.path, headers, req.body, content_type);
        }
        else if (req.method == "DELETE")
        {
            result = client_.Delete(req.path, headers);
        }
        else
        {
            response.error = "Unsupported HTTP method: " + req.method;
            return response;
        }

        if (result)
        {
            response.status = result->status;
            response.body = result->body;
            for (const auto& [key, value] : result->headers)
            {
                response.headers.push_back({key, value});
            }
        }
        else
        {
            response.error = httplib::to_string(result.error());
        }

        return response;
    }

    bool start_sse(
        const std::string& path,
        const std::vector<std::pair<std::string, std::string>>& headers,
        SSEEventCallback on_event,
        SSEErrorCallback on_error,
        SSECloseCallback on_close
    )
    {
        stop_sse();

        sse_running_ = true;
        sse_thread_ = std::thread([this, path, headers, on_event, on_error, on_close]()
        {
            run_sse(path, headers, on_event, on_error, on_close);
        });

        return true;
    }

    void stop_sse()
    {
        sse_running_ = false;
        if (sse_thread_.joinable())
        {
            sse_thread_.join();
        }
    }

    bool sse_connected() const
    {
        return sse_connected_;
    }

    void set_directory(const std::string& directory)
    {
        directory_ = directory;
    }

    void set_connection_timeout(int seconds)
    {
        connection_timeout_ = seconds;
        client_.set_connection_timeout(seconds, 0);
    }

    void set_read_timeout(int seconds)
    {
        read_timeout_ = seconds;
        client_.set_read_timeout(seconds, 0);
    }

  private:
    void run_sse(
        const std::string& path,
        const std::vector<std::pair<std::string, std::string>>& headers,
        SSEEventCallback on_event,
        SSEErrorCallback on_error,
        SSECloseCallback on_close
    )
    {
        // Create a separate client for SSE (long-lived connection)
        httplib::Client sse_client(host_, port_);
        // Use a long timeout for SSE (10 minutes) - 0 means "no timeout" but httplib
        // interprets that as 0 seconds which causes immediate failure
        sse_client.set_read_timeout(600, 0);  // 10 minutes
        sse_client.set_connection_timeout(30);

        // Build headers
        httplib::Headers http_headers;
        http_headers.insert({"Accept", "text/event-stream"});
        http_headers.insert({"Cache-Control", "no-cache"});
        http_headers.insert({"Connection", "keep-alive"});
        for (const auto& [key, value] : headers)
        {
            http_headers.insert({key, value});
        }
        if (!directory_.empty())
        {
            http_headers.insert({"x-opencode-directory", directory_});
        }

        SSEParser parser;
        sse_connected_ = true;

        auto result = sse_client.Get(
            path,
            http_headers,
            [this, &parser, &on_event](const char* data, size_t data_length) -> bool
            {
                if (!sse_running_)
                {
                    return false; // Stop receiving
                }

                std::string chunk(data, data_length);
                parser.feed(chunk, on_event);
                return true;
            }
        );

        sse_connected_ = false;

        if (sse_running_)
        {
            // Connection closed unexpectedly
            if (!result)
            {
                on_error(httplib::to_string(result.error()));
            }
        }

        on_close();
    }

    std::string host_;
    int port_;
    httplib::Client client_;
    std::string directory_;
    int connection_timeout_ = 30;
    int read_timeout_ = 30;

    std::atomic<bool> sse_running_{false};
    std::atomic<bool> sse_connected_{false};
    std::thread sse_thread_;
};

// =============================================================================
// HttpTransport Public Interface
// =============================================================================

HttpTransport::HttpTransport(const std::string& host, int port)
    : impl_(std::make_unique<Impl>(host, port))
{
}

HttpTransport::HttpTransport(
    const std::string& host,
    int port,
    const std::string& username,
    const std::string& password
)
    : impl_(std::make_unique<Impl>(host, port, username, password))
{
}

HttpTransport::~HttpTransport() = default;

HttpResponse HttpTransport::request(const HttpRequest& req)
{
    return impl_->request(req);
}

bool HttpTransport::start_sse(
    const std::string& path,
    const std::vector<std::pair<std::string, std::string>>& headers,
    SSEEventCallback on_event,
    SSEErrorCallback on_error,
    SSECloseCallback on_close
)
{
    return impl_->start_sse(path, headers, on_event, on_error, on_close);
}

void HttpTransport::stop_sse()
{
    impl_->stop_sse();
}

bool HttpTransport::sse_connected() const
{
    return impl_->sse_connected();
}

void HttpTransport::set_directory(const std::string& directory)
{
    impl_->set_directory(directory);
}

void HttpTransport::set_connection_timeout(int seconds)
{
    impl_->set_connection_timeout(seconds);
}

void HttpTransport::set_read_timeout(int seconds)
{
    impl_->set_read_timeout(seconds);
}

} // namespace opencode
