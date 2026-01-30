// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#include <opencode/server.hpp>
#include <opencode/process.hpp>

#include <chrono>
#include <stdexcept>
#include <thread>
#include <regex>

namespace opencode
{

Server::Server(std::string url, std::string hostname, int port, std::unique_ptr<Process> process)
    : url_(std::move(url)), hostname_(std::move(hostname)), port_(port), process_(std::move(process))
{
}

Server::~Server()
{
    stop();
}

Server::Server(Server&& other) noexcept
    : url_(std::move(other.url_)), hostname_(std::move(other.hostname_)), port_(other.port_),
      process_(std::move(other.process_))
{
    other.port_ = 0;
}

Server& Server::operator=(Server&& other) noexcept
{
    if (this != &other)
    {
        stop();
        url_ = std::move(other.url_);
        hostname_ = std::move(other.hostname_);
        port_ = other.port_;
        process_ = std::move(other.process_);
        other.port_ = 0;
    }
    return *this;
}

Server Server::spawn(const ServerOptions& opts)
{
    auto process = std::make_unique<Process>();

    // Build command line arguments
    std::vector<std::string> args;
    args.push_back("serve");
    args.push_back("--hostname");
    args.push_back(opts.hostname);
    args.push_back("--port");
    args.push_back(std::to_string(opts.port));

    if (opts.mdns)
    {
        args.push_back("--mdns");
    }

    // Build process options
    ProcessOptions proc_opts;
    proc_opts.redirect_stdout = true;
    proc_opts.redirect_stderr = false;
    proc_opts.redirect_stdin = false;
    proc_opts.inherit_environment = true;

    if (opts.working_directory)
    {
        proc_opts.working_directory = *opts.working_directory;
    }

    // Set environment variables for auth/config
    if (opts.config_json)
    {
        proc_opts.environment["OPENCODE_CONFIG_CONTENT"] = *opts.config_json;
    }
    if (opts.password)
    {
        proc_opts.environment["OPENCODE_SERVER_PASSWORD"] = *opts.password;
    }
    if (opts.username)
    {
        proc_opts.environment["OPENCODE_SERVER_USERNAME"] = *opts.username;
    }

    // Spawn the process
    process->spawn(opts.opencode_binary, args, proc_opts);

    // Wait for server to output the listening message
    // OpenCode outputs something like: "opencode server listening on http://127.0.0.1:4096"
    // or similar format based on the server implementation
    auto start_time = std::chrono::steady_clock::now();
    std::string accumulated_output;
    std::string detected_url;
    int detected_port = opts.port;
    std::string detected_hostname = opts.hostname;

    // Pattern to match server listening line
    // Common patterns:
    // - "Listening on http://127.0.0.1:4096"
    // - "Server running at http://localhost:4096"
    // - "opencode server listening on http://127.0.0.1:4096"
    std::regex url_pattern(R"((?:listening|running|started|bound)\s+(?:on|at)\s+(https?://[^\s]+))",
                          std::regex::icase);
    std::regex port_pattern(R"(:(\d+))");

    while (process->is_running())
    {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed >= opts.startup_timeout)
        {
            process->kill();
            throw std::runtime_error("Server startup timeout: did not detect listening message within " +
                                     std::to_string(opts.startup_timeout.count()) + "ms. Output: " + accumulated_output);
        }

        // Check for data with a short timeout
        if (process->stdout_pipe().has_data(100))
        {
            std::string line = process->stdout_pipe().read_line();
            accumulated_output += line;

            // Check for URL in output
            std::smatch match;
            if (std::regex_search(line, match, url_pattern))
            {
                detected_url = match[1].str();

                // Extract port from URL
                std::smatch port_match;
                if (std::regex_search(detected_url, port_match, port_pattern))
                {
                    detected_port = std::stoi(port_match[1].str());
                }

                // We found the listening message, server is ready
                break;
            }

            // Also check for simple port binding messages
            if (line.find(":" + std::to_string(opts.port)) != std::string::npos &&
                (line.find("listen") != std::string::npos ||
                 line.find("bound") != std::string::npos ||
                 line.find("server") != std::string::npos))
            {
                detected_url = "http://" + opts.hostname + ":" + std::to_string(opts.port);
                break;
            }
        }
    }

    // Check if process died during startup
    if (!process->is_running())
    {
        int exit_code = process->wait();
        throw std::runtime_error("Server process exited during startup with code " +
                                 std::to_string(exit_code) + ". Output: " + accumulated_output);
    }

    // Construct URL if not detected
    if (detected_url.empty())
    {
        detected_url = "http://" + opts.hostname + ":" + std::to_string(opts.port);
    }

    return Server(detected_url, detected_hostname, detected_port, std::move(process));
}

void Server::stop()
{
    if (process_ && process_->is_running())
    {
        // Try graceful termination first
        process_->terminate();

        // Wait up to 5 seconds for graceful exit
        auto start = std::chrono::steady_clock::now();
        while (process_->is_running())
        {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed > std::chrono::seconds(5))
            {
                // Force kill if still running
                process_->kill();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        process_->wait();
    }
}

void Server::force_stop()
{
    if (process_ && process_->is_running())
    {
        process_->kill();
        process_->wait();
    }
}

bool Server::running() const
{
    return process_ && process_->is_running();
}

std::string Server::url() const
{
    return url_;
}

std::string Server::hostname() const
{
    return hostname_;
}

int Server::port() const
{
    return port_;
}

int Server::pid() const
{
    return process_ ? process_->pid() : 0;
}

int Server::wait()
{
    if (process_)
    {
        return process_->wait();
    }
    return -1;
}

} // namespace opencode
