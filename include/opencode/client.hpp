// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <opencode/events.hpp>
#include <opencode/session.hpp>
#include <opencode/transport.hpp>
#include <opencode/types.hpp>

namespace opencode
{

// Forward declarations
class Server;

// =============================================================================
// Client Options
// =============================================================================

struct ClientOptions
{
    /// Explicit server URL - if set, connects to this URL instead of spawning
    /// Use for testing or connecting to remote/shared servers
    std::optional<std::string> base_url;

    /// Path to opencode executable (default: searches PATH)
    std::string opencode_path = "opencode";

    /// Working directory for the server
    std::optional<std::string> directory;

    /// Server startup timeout in milliseconds
    int startup_timeout_ms = 10000;

    /// Optional Basic auth credentials (username, password)
    std::optional<std::pair<std::string, std::string>> basic_auth;

    /// Default provider for messages (e.g., "anthropic", "openai")
    std::optional<std::string> default_provider;

    /// Default model for messages (e.g., "claude-sonnet-4", "gpt-4o")
    std::optional<std::string> default_model;

    /// Connection timeout in seconds
    int connection_timeout = 30;

    /// Read timeout in seconds
    int read_timeout = 300;
};

// =============================================================================
// Message Stream (for streaming responses)
// =============================================================================

/// Stream of message parts from a prompt response
class MessageStream
{
  public:
    class Iterator
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = MessageWithParts;
        using difference_type = std::ptrdiff_t;
        using pointer = const MessageWithParts*;
        using reference = const MessageWithParts&;

        Iterator();
        explicit Iterator(MessageStream* stream);
        reference operator*() const;
        pointer operator->() const;
        Iterator& operator++();
        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;

      private:
        MessageStream* stream_;
        std::optional<MessageWithParts> current_;
        bool at_end_ = false;
        void fetch_next();
    };

    MessageStream();
    ~MessageStream();
    MessageStream(const MessageStream&) = delete;
    MessageStream& operator=(const MessageStream&) = delete;
    MessageStream(MessageStream&&) noexcept;
    MessageStream& operator=(MessageStream&&) noexcept;

    Iterator begin();
    Iterator end();
    std::optional<MessageWithParts> next();
    void close();

  private:
    friend class Client;
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

// =============================================================================
// Event Stream (for SSE events)
// =============================================================================

/// Stream of events from the server (SSE)
class EventStream
{
  public:
    class Iterator
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Event;
        using difference_type = std::ptrdiff_t;
        using pointer = const Event*;
        using reference = const Event&;

        Iterator();
        explicit Iterator(EventStream* stream);
        reference operator*() const;
        pointer operator->() const;
        Iterator& operator++();
        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;

      private:
        EventStream* stream_;
        std::optional<Event> current_;
        bool at_end_ = false;
        void fetch_next();
    };

    EventStream();
    ~EventStream();
    EventStream(const EventStream&) = delete;
    EventStream& operator=(const EventStream&) = delete;
    EventStream(EventStream&&) noexcept;
    EventStream& operator=(EventStream&&) noexcept;

    Iterator begin();
    Iterator end();
    std::optional<Event> next();
    void close();

  private:
    friend class Client;
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

// =============================================================================
// Client
// =============================================================================

/// Client for interacting with an OpenCode server
///
/// The client automatically discovers a running server or spawns one.
///
/// Example usage:
/// @code
/// // Simple usage - auto-discovers or spawns server
/// opencode::Client client;
///
/// auto session = client.create_session("My Chat");
/// auto response = session.send("Hello, world!");
/// std::cout << response.text() << "\n";
/// @endcode
///
/// With options:
/// @code
/// opencode::Client client({
///     .default_provider = "anthropic",
///     .default_model = "claude-sonnet-4"
/// });
///
/// auto session = client.create_session();
/// session.send("What's 2+2?");
/// @endcode
class Client
{
  public:
    /// Create a client with default options (auto-discovers or spawns server)
    Client();

    /// Create a client with the given options
    explicit Client(ClientOptions opts);

    /// Create a client with a custom transport (for testing)
    Client(ClientOptions opts, std::unique_ptr<Transport> transport);

    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    // =========================================================================
    // Connection
    // =========================================================================

    /// Check if connected to server
    bool is_connected() const;

    /// Get the server URL
    std::string server_url() const;

    /// Check server health
    /// @return Health information
    /// @throws std::runtime_error on error
    HealthInfo health();

    // =========================================================================
    // Session Operations
    // =========================================================================

    /// Create a new session
    /// @param title Optional session title
    /// @return Session object for multi-turn conversation
    Session create_session(const std::string& title = {});

    /// Get an existing session by ID
    /// @param session_id Session ID
    /// @return Session object
    Session get_session(const std::string& session_id);

    /// List all sessions
    /// @return Vector of session info
    std::vector<SessionInfo> list_sessions();

    /// Delete a session
    /// @param session_id Session ID
    /// @return true if deleted
    bool delete_session(const std::string& session_id);

    // =========================================================================
    // Messages (low-level API)
    // =========================================================================

    /// Send a message to a session (blocking, waits for complete response)
    /// @param session_id Session ID
    /// @param prompt User prompt text
    /// @param provider_id Optional provider ID (e.g., "anthropic", "openai")
    /// @param model_id Optional model ID (e.g., "claude-sonnet-4", "gpt-4o")
    /// @return Complete assistant message with parts
    MessageWithParts send_message(
        const std::string& session_id,
        const std::string& prompt,
        const std::string& provider_id = {},
        const std::string& model_id = {}
    );

    /// Send a message with streaming callbacks
    /// Subscribes to SSE events to receive part updates in real-time
    /// @param session_id Session ID
    /// @param prompt User prompt text
    /// @param provider_id Optional provider ID
    /// @param model_id Optional model ID
    /// @param options Streaming callbacks (on_part, on_complete, on_error)
    void send_message_streaming(
        const std::string& session_id,
        const std::string& prompt,
        const std::string& provider_id,
        const std::string& model_id,
        StreamOptions options
    );

    /// Get messages for a session
    /// @param session_id Session ID
    /// @param limit Optional limit on number of messages
    /// @return Vector of messages with parts
    std::vector<MessageWithParts> get_messages(
        const std::string& session_id,
        std::optional<int> limit = std::nullopt
    );

    // =========================================================================
    // Session Control (low-level API)
    // =========================================================================

    /// Abort a session's current operation
    /// @param session_id Session ID
    /// @return true if successful
    bool abort_session(const std::string& session_id);

    /// Initialize a session with project analysis
    /// @param session_id Session ID
    /// @param provider_id Provider to use
    /// @param model_id Model to use
    /// @return true if successful
    bool init_session(
        const std::string& session_id,
        const std::string& provider_id,
        const std::string& model_id
    );

    /// Summarize a session
    /// @param session_id Session ID
    /// @param provider_id Provider to use
    /// @param model_id Model to use
    /// @return Summary text
    std::string summarize_session(
        const std::string& session_id,
        const std::string& provider_id,
        const std::string& model_id
    );

    /// Revert a message in a session
    /// @param session_id Session ID
    /// @param message_id Message ID
    /// @param part_id Optional part ID
    /// @return Updated session info
    SessionInfo revert_message(
        const std::string& session_id,
        const std::string& message_id,
        const std::string& part_id = {}
    );

    /// Restore reverted messages in a session
    /// @param session_id Session ID
    /// @return Updated session info
    SessionInfo unrevert_session(const std::string& session_id);

    /// Share a session publicly
    /// @param session_id Session ID
    /// @return Updated session info with share URL
    SessionInfo share_session(const std::string& session_id);

    /// Unshare a session
    /// @param session_id Session ID
    /// @return Updated session info
    SessionInfo unshare_session(const std::string& session_id);

    // =========================================================================
    // Permissions
    // =========================================================================

    /// List pending permission requests
    /// @return Vector of permission requests
    std::vector<PermissionRequest> list_permissions();

    /// Reply to a permission request
    /// @param reply Permission reply
    /// @return true if reply was accepted
    bool reply_permission(const PermissionReply& reply);

    // =========================================================================
    // Projects
    // =========================================================================

    /// List all projects
    /// @return Vector of projects
    std::vector<Project> list_projects();

    /// Get current project
    /// @return Current project
    Project current_project();

    // =========================================================================
    // Events
    // =========================================================================

    /// Subscribe to server events (SSE)
    /// @return Event stream
    EventStream subscribe_events();

    // =========================================================================
    // File Operations
    // =========================================================================

    /// List files in a directory
    /// @param path Directory path (default: current directory)
    /// @return Vector of file entries
    std::vector<FileEntry> list_files(const std::string& path = ".");

    /// Read a file's content
    /// @param path File path
    /// @return File content
    FileContent read_file(const std::string& path);

    /// Get file's git status
    /// @param path File path
    /// @return File status
    FileStatus file_status(const std::string& path);

    // =========================================================================
    // Find Operations
    // =========================================================================

    /// Search for text in files
    /// @param options Search options
    /// @return Search results
    TextSearchResult find_text(const TextSearchOptions& options);

    /// Find files by glob pattern
    /// @param options Search options
    /// @return Matching files
    std::vector<FileMatch> find_files(const FileSearchOptions& options);

    /// Find workspace symbols
    /// @param options Search options
    /// @return Matching symbols
    std::vector<SymbolMatch> find_symbols(const SymbolSearchOptions& options);

    // =========================================================================
    // App Information
    // =========================================================================

    /// List available AI providers and models
    /// @return Vector of providers with their models
    std::vector<ProviderDetails> list_providers();

    /// List available modes
    /// @return Vector of modes
    std::vector<ModeInfo> list_modes();

    /// List available agents
    /// @return Vector of agents
    std::vector<AgentInfo> list_agents();

    /// List available skills
    /// @return Vector of skills
    std::vector<SkillInfo> list_skills();

    /// Write to server log
    /// @param level Log level
    /// @param message Log message
    void log(LogLevel level, const std::string& message);

    // =========================================================================
    // Configuration
    // =========================================================================

    /// Get current configuration
    /// @return Current config
    Config get_config();

    /// Update configuration
    /// @param updates Config updates (only set fields are updated)
    /// @return Updated config
    Config update_config(const ConfigUpdate& updates);

    /// List configured providers
    /// @return Vector of config providers
    std::vector<ConfigProvider> list_config_providers();

    // =========================================================================
    // MCP (Model Context Protocol)
    // =========================================================================

    /// Get MCP server status
    /// @return MCP status with server list
    McpStatus mcp_status();

    /// Add an MCP server
    /// @param config Server configuration
    /// @return Added server info
    McpServer mcp_add(const McpServerConfig& config);

    /// Connect to an MCP server
    /// @param server_id Server ID
    /// @return Updated server info
    McpServer mcp_connect(const std::string& server_id);

    /// Disconnect from an MCP server
    /// @param server_id Server ID
    /// @return Updated server info
    McpServer mcp_disconnect(const std::string& server_id);

    // =========================================================================
    // Questions
    // =========================================================================

    /// List pending questions
    /// @return Vector of questions
    std::vector<Question> list_questions();

    /// Reply to a question
    /// @param reply Question reply
    /// @return true if reply was accepted
    bool reply_question(const QuestionReply& reply);

    /// Reject a question
    /// @param question_id Question ID
    /// @return true if rejected
    bool reject_question(const std::string& question_id);

    // =========================================================================
    // Worktrees
    // =========================================================================

    /// List worktrees
    /// @return Vector of worktrees
    std::vector<Worktree> list_worktrees();

    /// Create a worktree
    /// @param options Worktree creation options
    /// @return Created worktree
    Worktree create_worktree(const WorktreeCreate& options);

    /// Remove a worktree
    /// @param worktree_id Worktree ID
    /// @return true if removed
    bool remove_worktree(const std::string& worktree_id);

    /// Reset a worktree to clean state
    /// @param worktree_id Worktree ID
    /// @return Updated worktree
    Worktree reset_worktree(const std::string& worktree_id);

    // =========================================================================
    // Tools
    // =========================================================================

    /// List tool IDs (lightweight)
    /// @return Vector of tool IDs
    std::vector<std::string> list_tool_ids();

    /// List tools with full schema
    /// @return Vector of tools
    std::vector<ToolInfo> list_tools();

    // =========================================================================
    // LSP
    // =========================================================================

    /// Get LSP server status
    /// @return LSP status
    LspStatus lsp_status();

    // =========================================================================
    // Formatter
    // =========================================================================

    /// Get formatter status
    /// @return Formatter status
    FormatterStatus formatter_status();

    // =========================================================================
    // Auth
    // =========================================================================

    /// Set provider credentials
    /// @param provider_id Provider ID
    /// @param credentials Credentials
    /// @return Result
    AuthResult set_auth(const std::string& provider_id, const AuthCredentials& credentials);

    /// Remove provider credentials
    /// @param provider_id Provider ID
    /// @return Result
    AuthResult remove_auth(const std::string& provider_id);

    // =========================================================================
    // Parts
    // =========================================================================

    /// Delete a message part
    /// @param session_id Session ID
    /// @param message_id Message ID
    /// @param part_id Part ID
    /// @return true if deleted
    bool delete_part(
        const std::string& session_id,
        const std::string& message_id,
        const std::string& part_id
    );

    /// Update a message part
    /// @param session_id Session ID
    /// @param message_id Message ID
    /// @param part_id Part ID
    /// @param text New text content
    /// @return Updated part
    Part update_part(
        const std::string& session_id,
        const std::string& message_id,
        const std::string& part_id,
        const std::string& text
    );

    /// Get client options
    const ClientOptions& options() const;

    // =========================================================================
    // TUI (Terminal UI)
    // =========================================================================

    /// Open TUI
    void tui_open();

    /// Close TUI
    void tui_close();

    /// Focus TUI window
    void tui_focus();

    /// Unfocus TUI (blur)
    void tui_blur();

    /// Resize TUI
    /// @param width New width in columns
    /// @param height New height in rows
    void tui_resize(int width, int height);

    /// Select text in TUI
    /// @param selection Selection range (start/end positions)
    void tui_select(const TuiSelection& selection);

    /// Get TUI status
    /// @return TUI status info
    TuiStatus tui_status();

    /// Scroll TUI content
    /// @param lines Lines to scroll (positive = down, negative = up)
    void tui_scroll(int lines);

    /// Send input to TUI
    /// @param text Input text
    void tui_input(const std::string& text);

    /// Copy selection to clipboard
    void tui_copy();

    /// Paste from clipboard
    /// @return Pasted text
    std::string tui_paste();

    /// Clear TUI screen
    void tui_clear();

    /// Get current TUI render (visual state)
    /// @return TUI render with lines and size
    TuiRender tui_render();

    // =========================================================================
    // PTY (Pseudo-Terminal)
    // =========================================================================

    /// List all PTY sessions
    /// @return Vector of PTY session info
    std::vector<PtySession> list_pty_sessions();

    /// Create a new PTY session
    /// @param options Optional creation settings (shell, cwd, cols, rows, env)
    /// @return Created PTY session info
    PtySession create_pty(const PtyCreate& options = {});

    /// Write data to a PTY session
    /// @param pty_id PTY session ID
    /// @param data Data to write (e.g., command + newline)
    void pty_write(const std::string& pty_id, const std::string& data);

    /// Resize a PTY session
    /// @param pty_id PTY session ID
    /// @param cols New column count
    /// @param rows New row count
    /// @return Updated PTY session info
    PtySession pty_resize(const std::string& pty_id, int cols, int rows);

    /// Close a PTY session
    /// @param pty_id PTY session ID
    /// @return true if successfully closed
    bool close_pty(const std::string& pty_id);

  private:
    friend class Session;
    struct Impl;
    std::unique_ptr<Impl> impl_;

    /// Discover or spawn server
    void connect();
};

} // namespace opencode
