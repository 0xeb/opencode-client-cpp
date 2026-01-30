// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <opencode/types.hpp>

namespace opencode
{

// Forward declaration
class Client;

// =============================================================================
// Session
// =============================================================================

/// A conversation session with the OpenCode server
///
/// Session objects are created via Client::create_session() and provide
/// a clean interface for multi-turn conversations without needing to
/// track session IDs manually.
///
/// Example:
/// @code
/// opencode::Client client;
/// auto session = client.create_session("My Chat");
///
/// auto r1 = session.send("What's 2 + 2?");
/// auto r2 = session.send("Multiply that by 10");
/// std::cout << r2.text() << "\n";  // "40"
/// @endcode
class Session
{
  public:
    Session(Client* client, SessionInfo info);
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) noexcept;
    Session& operator=(Session&&) noexcept;

    // =========================================================================
    // Properties
    // =========================================================================

    /// Get the session ID
    const std::string& id() const { return info_.id; }

    /// Get the session title
    const std::string& title() const { return info_.title; }

    /// Get the full session info
    const SessionInfo& info() const { return info_; }

    // =========================================================================
    // Messaging
    // =========================================================================

    /// Send a message and wait for complete response
    /// @param prompt The message text
    /// @return Complete assistant response
    MessageWithParts send(const std::string& prompt);

    /// Send a message with specific model
    /// @param prompt The message text
    /// @param provider_id Provider (e.g., "anthropic", "openai")
    /// @param model_id Model (e.g., "claude-sonnet-4", "gpt-4o")
    /// @return Complete assistant response
    MessageWithParts send(
        const std::string& prompt,
        const std::string& provider_id,
        const std::string& model_id
    );

    /// Send a message with streaming callbacks
    /// Receive updates as they arrive via callbacks
    /// @param prompt The message text
    /// @param options Streaming callbacks (on_part, on_complete, on_error)
    void send_streaming(const std::string& prompt, StreamOptions options);

    /// Send a message with streaming and specific model
    /// @param prompt The message text
    /// @param provider_id Provider (e.g., "anthropic", "openai")
    /// @param model_id Model (e.g., "claude-sonnet-4", "gpt-4o")
    /// @param options Streaming callbacks
    void send_streaming(
        const std::string& prompt,
        const std::string& provider_id,
        const std::string& model_id,
        StreamOptions options
    );

    /// Get message history
    /// @param limit Optional limit on number of messages
    /// @return Vector of messages with parts
    std::vector<MessageWithParts> messages(std::optional<int> limit = std::nullopt);

    // =========================================================================
    // Control
    // =========================================================================

    /// Abort the current operation in this session
    /// Stops any running AI generation
    /// @return true if abort was successful
    bool abort();

    /// Initialize the session with project analysis
    /// Creates AGENTS.md file with project context
    /// @param provider_id Provider to use
    /// @param model_id Model to use
    /// @return true if init was successful
    bool init(const std::string& provider_id, const std::string& model_id);

    /// Summarize the session conversation
    /// @param provider_id Provider to use
    /// @param model_id Model to use
    /// @return Summary text
    std::string summarize(const std::string& provider_id, const std::string& model_id);

    // =========================================================================
    // History
    // =========================================================================

    /// Revert a message (undo)
    /// @param message_id Message ID to revert
    /// @param part_id Optional specific part to revert
    /// @return Updated session info
    SessionInfo revert(const std::string& message_id, const std::string& part_id = {});

    /// Restore all reverted messages (undo the undo)
    /// @return Updated session info
    SessionInfo unrevert();

    // =========================================================================
    // Sharing
    // =========================================================================

    /// Share this session publicly
    /// @return Updated session info with share URL
    SessionInfo share();

    /// Unshare this session (make private again)
    /// @return Updated session info
    SessionInfo unshare();

    // =========================================================================
    // Lifecycle
    // =========================================================================

    /// Delete this session from the server
    /// After calling this, the session object should not be used
    bool destroy();

  private:
    Client* client_;
    SessionInfo info_;
};

} // namespace opencode
