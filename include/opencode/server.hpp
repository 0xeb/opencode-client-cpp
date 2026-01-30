// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace opencode
{

// Forward declaration
class Process;

/// Options for spawning an OpenCode server
struct ServerOptions
{
    /// Path to the opencode binary (default: "opencode" - searches PATH)
    std::string opencode_binary = "opencode";

    /// Hostname to bind to (default: localhost)
    std::string hostname = "127.0.0.1";

    /// Port to bind to (default: 4096)
    int port = 4096;

    /// Enable mDNS discovery
    bool mdns = false;

    /// Optional JSON config to inject via OPENCODE_CONFIG_CONTENT
    std::optional<std::string> config_json;

    /// Optional password for Basic auth (via OPENCODE_SERVER_PASSWORD)
    std::optional<std::string> password;

    /// Optional username for Basic auth (via OPENCODE_SERVER_USERNAME, default: "opencode")
    std::optional<std::string> username;

    /// Working directory for the server process
    std::optional<std::string> working_directory;

    /// Timeout for waiting for server to start (default: 30 seconds)
    std::chrono::milliseconds startup_timeout{30000};
};

/// Manages a local OpenCode server process
///
/// Example usage:
/// @code
/// ServerOptions opts;
/// opts.port = 4096;
/// opts.working_directory = "/my/project";
///
/// auto server = Server::spawn(opts);
/// std::cout << "Server running at " << server.url() << std::endl;
///
/// // Use client to interact with server...
///
/// server.stop(); // Graceful shutdown
/// @endcode
class Server
{
  public:
    Server() = default;
    ~Server();

    // Non-copyable, movable
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) noexcept;
    Server& operator=(Server&&) noexcept;

    /// Spawn a new OpenCode server process
    /// @param opts Server configuration
    /// @return Running Server instance
    /// @throws std::runtime_error if spawn fails or server doesn't start within timeout
    static Server spawn(const ServerOptions& opts = {});

    /// Stop the server gracefully (SIGTERM)
    /// Waits up to 5 seconds for graceful shutdown before force killing
    void stop();

    /// Force stop the server (SIGKILL)
    void force_stop();

    /// Check if the server process is still running
    bool running() const;

    /// Get the server's base URL (e.g., "http://127.0.0.1:4096")
    std::string url() const;

    /// Get the server's hostname
    std::string hostname() const;

    /// Get the server's port
    int port() const;

    /// Get the process ID
    int pid() const;

    /// Wait for the server process to exit
    /// @return Exit code
    int wait();

  private:
    explicit Server(std::string url, std::string hostname, int port, std::unique_ptr<Process> process);

    std::string url_;
    std::string hostname_;
    int port_ = 0;
    std::unique_ptr<Process> process_;
};

} // namespace opencode
