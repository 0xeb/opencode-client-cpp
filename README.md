# OpenCode Client C++

A C++ client SDK for the [OpenCode](https://github.com/sst/opencode) AI coding assistant.

- **Dedicated servers**: Each client spawns its own server (RAII cleanup)
- **Session-based API**: Clean `Session` objects for multi-turn conversations
- **Streaming**: Real-time token streaming with callbacks
- **SSE events**: Subscribe to server events
- **Cross-platform**: Windows, macOS, Linux

## Requirements

- C++23 compiler (GCC 13+, Clang 16+, MSVC 19.37+)
- CMake 3.23+

Dependencies (fetched automatically):
- [nlohmann/json](https://github.com/nlohmann/json)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Options:
- `OPENCODE_CLIENT_BUILD_EXAMPLES=ON` (default)
- `OPENCODE_CLIENT_BUILD_TESTS=ON` (default)
- `OPENCODE_CLIENT_FETCH_DEPS=ON` (default) - set OFF to use system packages

## Quick Start

### Simple chat

```cpp
#include <opencode/opencode.hpp>
#include <iostream>

int main() {
    // Each client spawns its own dedicated server
    opencode::Client client;

    // Create a session and chat
    auto session = client.create_session("My Chat");
    auto response = session.send("What is 2 + 2?");

    std::cout << response.text() << "\n";  // "4"

    session.destroy();
    return 0;
}
```

### With model selection

```cpp
opencode::Client client({
    .default_provider = "anthropic",
    .default_model = "claude-sonnet-4"
});

auto session = client.create_session("Code Review");
auto response = session.send("Review this code: ...");

// Or override per-message
auto response2 = session.send("Explain simply", "openai", "gpt-4o");
```

### Streaming responses

```cpp
session.send_streaming("Count to 5", {
    .on_part = [](const opencode::Part& part) {
        if (auto* text = std::get_if<opencode::TextPart>(&part)) {
            if (text->is_delta)  // Only print streaming deltas
                std::cout << text->text << std::flush;
        }
    },
    .on_complete = [](const opencode::MessageWithParts& msg) {
        std::cout << "\n[Done: " << msg.tokens()->output << " tokens]\n";
    },
    .on_error = [](const std::string& error) {
        std::cerr << "Error: " << error << "\n";
    }
});
```

### Subscribing to events

```cpp
auto events = client.subscribe_events();

for (const auto& event : events) {
    // Clean type-safe access with try_as<T> (inspired by copilot-sdk-cpp)
    if (auto* e = opencode::try_as<opencode::SessionCreatedEvent>(event)) {
        std::cout << "Session created: " << e->session.id << "\n";
    }
    else if (auto* e = opencode::try_as<opencode::PermissionAskedEvent>(event)) {
        // Auto-approve permissions
        client.reply_permission({e->request.id, opencode::PermissionAction::Always});
    }
    else if (opencode::is<opencode::ServerHeartbeatEvent>(event)) {
        // Just check type without accessing data
    }
}
```

### Connect to existing server

```cpp
// Skip spawning, connect to specific URL (e.g., remote or shared server)
opencode::Client client({.base_url = "http://192.168.1.100:4096"});
```

## API Reference

### Client

```cpp
Client();                              // Spawns dedicated server
Client(ClientOptions opts);            // With options (base_url skips spawn)

// Sessions
Session create_session(title = "");    // Returns Session object
Session get_session(session_id);
std::vector<SessionInfo> list_sessions();
bool delete_session(session_id);

// Low-level message API
MessageWithParts send_message(session_id, prompt, provider = "", model = "");
void send_message_streaming(session_id, prompt, provider, model, StreamOptions);
std::vector<MessageWithParts> get_messages(session_id, limit = nullopt);

// Permissions
std::vector<PermissionRequest> list_permissions();
bool reply_permission(PermissionReply);

// Projects
std::vector<Project> list_projects();
Project current_project();

// Events & Health
EventStream subscribe_events();
HealthInfo health();

// File Operations
std::vector<FileEntry> list_files(path = ".");
FileContent read_file(path);
FileStatus file_status(path);

// Find Operations
TextSearchResult find_text(TextSearchOptions);
std::vector<FileMatch> find_files(FileSearchOptions);
std::vector<SymbolMatch> find_symbols(SymbolSearchOptions);

// App Information
std::vector<ProviderDetails> list_providers();
std::vector<ModeInfo> list_modes();
std::vector<AgentInfo> list_agents();
std::vector<SkillInfo> list_skills();
void log(LogLevel, message);

// Configuration
Config get_config();
Config update_config(ConfigUpdate);
std::vector<ConfigProvider> list_config_providers();

// MCP (Model Context Protocol)
McpStatus mcp_status();
McpServer mcp_add(McpServerConfig);
McpServer mcp_connect(server_id);
McpServer mcp_disconnect(server_id);

// Questions
std::vector<Question> list_questions();
bool reply_question(QuestionReply);
bool reject_question(question_id);

// Worktrees
std::vector<Worktree> list_worktrees();
Worktree create_worktree(WorktreeCreate);
bool remove_worktree(worktree_id);
Worktree reset_worktree(worktree_id);

// Tools
std::vector<std::string> list_tool_ids();
std::vector<ToolInfo> list_tools();

// LSP & Formatter
LspStatus lsp_status();
FormatterStatus formatter_status();

// Auth
AuthResult set_auth(provider_id, AuthCredentials);
AuthResult remove_auth(provider_id);

// Parts (message part editing)
bool delete_part(session_id, message_id, part_id);
Part update_part(session_id, message_id, part_id, text);

// TUI (Terminal UI)
void tui_open();
void tui_close();
void tui_focus();
void tui_blur();
void tui_resize(int width, int height);
void tui_select(const TuiSelection& selection);
TuiStatus tui_status();
void tui_scroll(int lines);
void tui_input(const std::string& text);
void tui_copy();
std::string tui_paste();
void tui_clear();
TuiRender tui_render();

// PTY (Pseudo-Terminal)
std::vector<PtySession> list_pty_sessions();
PtySession create_pty(const PtyCreate& options = {});
void pty_write(const std::string& pty_id, const std::string& data);
PtySession pty_resize(const std::string& pty_id, int cols, int rows);
bool close_pty(const std::string& pty_id);
```

### Session

```cpp
const std::string& id() const;
const std::string& title() const;

// Blocking send
MessageWithParts send(prompt);
MessageWithParts send(prompt, provider_id, model_id);

// Streaming send
void send_streaming(prompt, StreamOptions);
void send_streaming(prompt, provider_id, model_id, StreamOptions);

// History
std::vector<MessageWithParts> messages(limit = nullopt);

// Control
bool abort();                                    // Stop running operation
bool init(provider_id, model_id);               // Create AGENTS.md
std::string summarize(provider_id, model_id);   // Summarize conversation

// Undo/Redo
SessionInfo revert(message_id, part_id = "");   // Undo message
SessionInfo unrevert();                          // Restore reverted

// Sharing
SessionInfo share();                             // Share publicly
SessionInfo unshare();                           // Make private

// Cleanup
bool destroy();
```

### MessageWithParts helpers

```cpp
std::string text() const;                    // Get concatenated text
bool is_assistant() const;                   // Check if assistant message
std::optional<TokenInfo> tokens() const;     // Get token counts
std::optional<double> cost() const;          // Get cost
```


## Examples

See `examples/` directory:

**Chat & Sessions**
- `multi_turn_chat.cpp` - Basic multi-turn conversation
- `streaming_chat.cpp` - Real-time streaming output
- `interactive_chat.cpp` - Interactive REPL
- `session_resume.cpp` - Resume previous sessions

**Events & Permissions**
- `event_monitor.cpp` - SSE event subscription
- `permission_handler.cpp` - Auto-approve permissions

**Model & Provider Management**
- `model_comparison.cpp` - Compare different models
- `config_manager.cpp` - View and update configuration

**File & Search Operations**
- `file_browser.cpp` - Browse and read project files
- `code_search.cpp` - Search for text, files, and symbols

**Tools & Integrations**
- `tools_explorer.cpp` - List and inspect available tools
- `mcp_manager.cpp` - Manage MCP servers and resources
- `project_info.cpp` - Display project information

## Testing

The SDK includes comprehensive E2E tests. Requires a running OpenCode server.

```bash
# Basic E2E tests
./build/Release/opencode-e2e-test

# Extended E2E tests (33+ test cases)
./build/Release/opencode-e2e-extended

# Smoke test (no server required)
./build/Release/opencode-smoke
```

Set environment variables to customize test provider/model:
```bash
export OPENCODE_TEST_PROVIDER=anthropic
export OPENCODE_TEST_MODEL=claude-sonnet-4
```

## License

Copyright 2025 Elias Bachaalany

Licensed under the MIT License.

This is a C++ client SDK for [OpenCode](https://github.com/sst/opencode) by SST.
