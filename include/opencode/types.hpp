// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace opencode
{

// =============================================================================
// Time Info
// =============================================================================

struct TimeInfo
{
    int64_t created = 0;
    int64_t updated = 0;
    std::optional<int64_t> compacting;
    std::optional<int64_t> archived;
    std::optional<int64_t> completed;
};

// =============================================================================
// Session
// =============================================================================

struct SessionSummary
{
    int additions = 0;
    int deletions = 0;
    int files = 0;
};

struct SessionInfo
{
    std::string id;
    std::string slug;
    std::string project_id;
    std::string directory;
    std::string title;
    std::string version;
    TimeInfo time;
    std::optional<std::string> parent_id;
    std::optional<SessionSummary> summary;
    std::optional<std::string> share_url;  // URL if session is shared publicly
};

// =============================================================================
// Project
// =============================================================================

struct ProjectIcon
{
    std::optional<std::string> url;
    std::optional<std::string> override_;
    std::optional<std::string> color;
};

struct ProjectCommands
{
    std::optional<std::string> start;
};

struct Project
{
    std::string id;
    std::string worktree;
    std::optional<std::string> vcs;
    std::optional<std::string> name;
    std::optional<ProjectIcon> icon;
    std::optional<ProjectCommands> commands;
    TimeInfo time;
    std::vector<std::string> sandboxes;
};

// =============================================================================
// Message Parts
// =============================================================================

struct TextPart
{
    std::string type = "text";
    std::string id;
    std::string text;
    bool is_delta = false;  // True if text represents a streaming delta, not full content
};

struct FilePart
{
    std::string type = "file";
    std::string id;
    std::string file;
    std::optional<std::string> content;
};

struct ToolState
{
    std::string status; // "pending", "running", "completed", "error"
    std::optional<std::string> error;
};

struct ToolPart
{
    std::string type = "tool";
    std::string id;
    std::string tool;
    std::map<std::string, std::string> input;
    std::optional<ToolState> state;
};

struct ReasoningPart
{
    std::string type = "reasoning";
    std::string id;
    std::string text;
};

// Generic part variant (simplified - can be extended)
using Part = std::variant<TextPart, FilePart, ToolPart, ReasoningPart>;

// =============================================================================
// Messages
// =============================================================================

struct ModelInfo
{
    std::string provider_id;
    std::string model_id;
};

struct PathInfo
{
    std::string cwd;
    std::string root;
};

struct TokenInfo
{
    int input = 0;
    int output = 0;
    int reasoning = 0;
    struct Cache
    {
        int read = 0;
        int write = 0;
    } cache;
};

struct UserMessage
{
    std::string id;
    std::string session_id;
    std::string role = "user";
    TimeInfo time;
    std::string agent;
    ModelInfo model;
    std::optional<std::string> system;
    std::optional<std::string> variant;
};

struct AssistantMessage
{
    std::string id;
    std::string session_id;
    std::string role = "assistant";
    TimeInfo time;
    std::string parent_id;
    std::string model_id;
    std::string provider_id;
    std::string mode;
    std::string agent;
    PathInfo path;
    double cost = 0.0;
    TokenInfo tokens;
    std::optional<std::string> finish;
    std::optional<bool> summary;
};

// Combined message type
using Message = std::variant<UserMessage, AssistantMessage>;

// Message with parts (as returned by API)
struct MessageWithParts
{
    Message info;
    std::vector<Part> parts;

    /// Get the message ID
    std::string id() const
    {
        return std::visit([](const auto& m) { return m.id; }, info);
    }

    /// Extract all text content from the message
    std::string text() const
    {
        std::string result;
        for (const auto& part : parts)
        {
            if (auto* t = std::get_if<TextPart>(&part))
            {
                if (!result.empty())
                    result += "\n";
                result += t->text;
            }
        }
        return result;
    }

    /// Check if this is an assistant message
    bool is_assistant() const
    {
        return std::holds_alternative<AssistantMessage>(info);
    }

    /// Get token usage (if assistant message)
    std::optional<TokenInfo> tokens() const
    {
        if (auto* a = std::get_if<AssistantMessage>(&info))
            return a->tokens;
        return std::nullopt;
    }

    /// Get cost (if assistant message)
    std::optional<double> cost() const
    {
        if (auto* a = std::get_if<AssistantMessage>(&info))
            return a->cost;
        return std::nullopt;
    }
};

// =============================================================================
// Session Status
// =============================================================================

struct SessionStatus
{
    std::string status; // "idle", "generating", "waiting", etc.
    std::optional<std::string> message_id;
    std::optional<std::string> part_id;
};

// =============================================================================
// Permissions
// =============================================================================

struct PermissionRequest
{
    std::string id;
    std::string session_id;
    std::string permission;
    std::vector<std::string> patterns;
    std::optional<std::string> tool_message_id;
    std::optional<std::string> tool_call_id;
    TimeInfo time;
};

enum class PermissionAction
{
    Once,
    Always,
    Reject
};

struct PermissionReply
{
    std::string request_id;
    PermissionAction action = PermissionAction::Once;
    std::optional<std::string> message;
};

// =============================================================================
// Health
// =============================================================================

struct HealthInfo
{
    bool healthy = false;
    std::string version;
};

// =============================================================================
// Config
// =============================================================================

struct ProviderInfo
{
    std::string id;
    std::string name;
    std::vector<std::string> models;
};

// =============================================================================
// Streaming Callbacks
// =============================================================================

/// Called when a message part is created or updated during streaming
using StreamPartCallback = std::function<void(const Part& part)>;

/// Called when the complete message is received
using StreamCompleteCallback = std::function<void(const MessageWithParts& message)>;

/// Called on error during streaming
using StreamErrorCallback = std::function<void(const std::string& error)>;

/// Streaming options for send_streaming()
struct StreamOptions
{
    /// Called for each part update (text deltas, tool calls, etc.)
    StreamPartCallback on_part;

    /// Called when complete message is ready
    StreamCompleteCallback on_complete;

    /// Called on error
    StreamErrorCallback on_error;
};

// =============================================================================
// File Operations
// =============================================================================

struct FileEntry
{
    std::string name;
    std::string path;
    bool is_directory = false;
    std::optional<int64_t> size;
    std::optional<int64_t> modified;
};

struct FileContent
{
    std::string path;
    std::string content;
    std::optional<std::string> encoding;  // "utf-8", "base64"
};

struct FileStatus
{
    std::string path;
    std::string status;  // "modified", "added", "deleted", "untracked", "clean"
    std::optional<int> additions;
    std::optional<int> deletions;
};

// =============================================================================
// Find Operations
// =============================================================================

struct TextMatch
{
    std::string path;
    int line = 0;
    int column = 0;
    std::string text;   // Matching line content
    std::string match;  // The actual match
};

struct TextSearchResult
{
    std::vector<TextMatch> matches;
    int total_matches = 0;
    bool truncated = false;
};

struct TextSearchOptions
{
    std::string pattern;
    std::optional<std::string> glob;  // File pattern filter
    std::optional<int> limit;
    bool regex = false;
    bool case_sensitive = true;
};

struct FileMatch
{
    std::string path;
    std::string name;
    bool is_directory = false;
};

struct FileSearchOptions
{
    std::string pattern;  // Glob pattern
    std::optional<int> limit;
};

struct SymbolMatch
{
    std::string name;
    std::string kind;  // "function", "class", "variable", etc.
    std::string path;
    int line = 0;
    int column = 0;
    std::optional<std::string> container;  // Parent symbol
};

struct SymbolSearchOptions
{
    std::string query;
    std::optional<int> limit;
};

// =============================================================================
// App Information
// =============================================================================

struct ModelDetails
{
    std::string id;
    std::string name;
    std::optional<std::string> description;
    std::optional<int> context_length;
    std::optional<double> input_cost;   // Per 1M tokens
    std::optional<double> output_cost;  // Per 1M tokens
};

struct ProviderDetails
{
    std::string id;
    std::string name;
    std::vector<ModelDetails> models;
    bool configured = false;
    std::optional<std::string> error;
};

struct ModeInfo
{
    std::string id;
    std::string name;
    std::optional<std::string> description;
};

struct AgentInfo
{
    std::string id;
    std::string name;
    std::optional<std::string> description;
};

struct SkillInfo
{
    std::string id;
    std::string name;
    std::optional<std::string> description;
    std::vector<std::string> commands;
};

enum class LogLevel
{
    Debug,
    Info,
    Warn,
    Error
};

// =============================================================================
// Configuration
// =============================================================================

struct ConfigProvider
{
    std::string id;
    bool enabled = false;
    std::optional<std::string> api_key_env;
    bool has_key = false;
};

struct Config
{
    std::optional<std::string> default_provider;
    std::optional<std::string> default_model;
    std::optional<bool> auto_approve;
    std::optional<int> max_tokens;
    std::optional<double> temperature;
    std::optional<std::string> theme;
    std::optional<bool> show_cost;
    std::vector<ConfigProvider> providers;
};

struct ConfigUpdate
{
    std::optional<std::string> default_provider;
    std::optional<std::string> default_model;
    std::optional<bool> auto_approve;
    std::optional<int> max_tokens;
    std::optional<double> temperature;
};

// =============================================================================
// MCP (Model Context Protocol)
// =============================================================================

struct McpTool
{
    std::string name;
    std::optional<std::string> description;
};

struct McpResource
{
    std::string uri;
    std::string name;
    std::optional<std::string> description;
    std::optional<std::string> mime_type;
};

struct McpServer
{
    std::string id;
    std::string name;
    std::string status;  // "connected", "disconnected", "error", "connecting"
    std::optional<std::string> error;
    std::vector<McpTool> tools;
    std::vector<McpResource> resources;
};

struct McpServerConfig
{
    std::string name;
    std::string command;
    std::vector<std::string> args;
    std::map<std::string, std::string> env;
};

struct McpStatus
{
    std::vector<McpServer> servers;
};

// =============================================================================
// Questions
// =============================================================================

struct QuestionOption
{
    std::string label;
    std::string value;
    std::optional<std::string> description;
};

struct Question
{
    std::string id;
    std::string session_id;
    std::string text;
    std::string type;  // "text", "choice", "confirm"
    std::vector<QuestionOption> options;
    std::optional<std::string> default_value;
    TimeInfo time;
};

struct QuestionReply
{
    std::string question_id;
    std::string answer;
};

// =============================================================================
// Worktrees
// =============================================================================

struct Worktree
{
    std::string id;
    std::string path;
    std::string branch;
    bool is_main = false;
    std::optional<std::string> commit;
    std::optional<bool> is_bare;
    std::optional<bool> is_detached;
    TimeInfo time;
};

struct WorktreeCreate
{
    std::string branch;
    std::optional<std::string> path;
    std::optional<std::string> base;
    bool create_branch = false;
};

// =============================================================================
// Tools
// =============================================================================

struct ToolParameter
{
    std::string name;
    std::string type;  // "string", "number", "boolean", "array", "object"
    std::optional<std::string> description;
    bool required = false;
    std::optional<std::string> default_value;
};

struct ToolInfo
{
    std::string id;
    std::string name;
    std::optional<std::string> description;
    std::vector<ToolParameter> parameters;
    std::optional<std::string> category;
    bool enabled = true;
};

// =============================================================================
// LSP Status
// =============================================================================

struct LspServer
{
    std::string language;
    std::string name;
    std::string status;  // "running", "stopped", "error"
    std::optional<std::string> version;
    std::optional<std::string> error;
    std::optional<int> pid;
};

struct LspStatus
{
    std::vector<LspServer> servers;
};

// =============================================================================
// Formatter Status
// =============================================================================

struct Formatter
{
    std::string language;
    std::string name;
    std::string status;  // "available", "unavailable"
    std::optional<std::string> version;
    std::optional<std::string> error;
};

struct FormatterStatus
{
    std::vector<Formatter> formatters;
};

// =============================================================================
// Auth
// =============================================================================

struct AuthCredentials
{
    std::string api_key;
    std::optional<std::string> api_base;
    std::optional<std::string> organization;
};

struct AuthResult
{
    bool success = false;
    std::optional<std::string> error;
};

// =============================================================================
// TUI (Terminal UI)
// =============================================================================

struct TuiSize
{
    int width = 0;
    int height = 0;
};

struct TuiPosition
{
    int x = 0;
    int y = 0;
};

struct TuiSelection
{
    TuiPosition start;
    TuiPosition end;
};

struct TuiStatus
{
    bool open = false;
    bool focused = false;
    TuiSize size;
    std::optional<TuiSelection> selection;
};


struct TuiRender
{
    std::vector<std::string> lines;  // Terminal lines (may contain ANSI codes)
    TuiSize size;
};

// =============================================================================
// PTY (Pseudo-Terminal)
// =============================================================================

struct PtySession
{
    std::string id;
    std::string shell;                        // e.g., "/bin/bash"
    int pid = 0;
    int cols = 80;
    int rows = 24;
    std::string status;                       // "running", "exited"
    std::optional<int> exit_code;
    TimeInfo time;
};

struct PtyCreate
{
    std::optional<std::string> shell;         // Default: user's shell
    std::optional<std::string> cwd;           // Working directory
    std::optional<int> cols;
    std::optional<int> rows;
    std::map<std::string, std::string> env;
};

// =============================================================================
// Error Types
// =============================================================================

struct ApiError
{
    std::string message;
    int status_code = 0;
    bool is_retryable = false;
    std::optional<std::string> response_body;
};

struct BadRequestError
{
    std::string error;
};

struct NotFoundError
{
    std::string error;
};

// =============================================================================
// JSON Helpers (to be implemented in types.cpp)
// =============================================================================

// Forward declare JSON conversion functions
// Implementation uses nlohmann::json

std::string permission_action_to_string(PermissionAction action);
PermissionAction string_to_permission_action(const std::string& str);

} // namespace opencode
