// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace opencode
{

// =============================================================================
// HTTP Request/Response
// =============================================================================

struct HttpRequest
{
    std::string method;
    std::string path; // Path portion only, not full URL
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
    std::optional<std::string> content_type;
};

struct HttpResponse
{
    int status = 0;
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string error; // Non-empty if request failed
};

// =============================================================================
// SSE Event
// =============================================================================

struct SSEEvent
{
    std::string event; // Event type
    std::string data;  // Event data (may span multiple lines)
    std::string id;    // Optional event ID
    int retry = 0;     // Retry interval in ms (0 = not specified)
};

// =============================================================================
// SSE Callbacks
// =============================================================================

using SSEEventCallback = std::function<void(const SSEEvent&)>;
using SSEErrorCallback = std::function<void(const std::string& error)>;
using SSECloseCallback = std::function<void()>;

// =============================================================================
// Transport Interface
// =============================================================================

/// Abstract transport interface for HTTP/SSE operations
class Transport
{
  public:
    virtual ~Transport() = default;

    /// Execute an HTTP request
    /// @param req The request to execute
    /// @return Response from server
    virtual HttpResponse request(const HttpRequest& req) = 0;

    /// Start an SSE connection
    /// @param path Path to SSE endpoint
    /// @param headers Additional headers
    /// @param on_event Callback for each SSE event
    /// @param on_error Callback for errors
    /// @param on_close Callback when connection closes
    /// @return true if connection started successfully
    virtual bool start_sse(
        const std::string& path,
        const std::vector<std::pair<std::string, std::string>>& headers,
        SSEEventCallback on_event,
        SSEErrorCallback on_error,
        SSECloseCallback on_close
    ) = 0;

    /// Stop the SSE connection
    virtual void stop_sse() = 0;

    /// Check if SSE is connected
    virtual bool sse_connected() const = 0;
};

// =============================================================================
// HTTP Transport Implementation
// =============================================================================

/// HTTP transport using cpp-httplib
class HttpTransport : public Transport
{
  public:
    /// Create transport for a server
    /// @param host Hostname (e.g., "127.0.0.1")
    /// @param port Port number (e.g., 4096)
    HttpTransport(const std::string& host, int port);

    /// Create transport with Basic auth
    /// @param host Hostname
    /// @param port Port
    /// @param username Basic auth username
    /// @param password Basic auth password
    HttpTransport(
        const std::string& host,
        int port,
        const std::string& username,
        const std::string& password
    );

    ~HttpTransport() override;

    // Non-copyable, non-movable (owns thread)
    HttpTransport(const HttpTransport&) = delete;
    HttpTransport& operator=(const HttpTransport&) = delete;
    HttpTransport(HttpTransport&&) = delete;
    HttpTransport& operator=(HttpTransport&&) = delete;

    HttpResponse request(const HttpRequest& req) override;

    bool start_sse(
        const std::string& path,
        const std::vector<std::pair<std::string, std::string>>& headers,
        SSEEventCallback on_event,
        SSEErrorCallback on_error,
        SSECloseCallback on_close
    ) override;

    void stop_sse() override;
    bool sse_connected() const override;

    /// Set the x-opencode-directory header for all requests
    void set_directory(const std::string& directory);

    /// Set connection timeout in seconds
    void set_connection_timeout(int seconds);

    /// Set read timeout in seconds
    void set_read_timeout(int seconds);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// SSE Parser
// =============================================================================

/// Parser for Server-Sent Events stream
class SSEParser
{
  public:
    SSEParser() = default;

    /// Feed data to the parser
    /// @param data Incoming data chunk
    /// @param on_event Callback for each complete event
    void feed(const std::string& data, const std::function<void(const SSEEvent&)>& on_event);

    /// Reset parser state
    void reset();

  private:
    std::string buffer_;
    SSEEvent current_event_;
};

} // namespace opencode
