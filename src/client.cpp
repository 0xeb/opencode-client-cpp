// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#include <opencode/client.hpp>
#include <opencode/server.hpp>
#include <opencode/transport.hpp>

#include <nlohmann/json.hpp>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <regex>
#include <stdexcept>
#include <thread>
#include <chrono>

namespace opencode
{

using json = nlohmann::json;

// =============================================================================
// JSON Helpers
// =============================================================================

namespace
{

std::pair<std::string, int> parse_url(const std::string& url)
{
    // Parse URL to extract host and port
    // Expected format: http(s)://host:port or http(s)://host
    std::regex url_regex(R"(^https?://([^:/]+)(?::(\d+))?)");
    std::smatch match;
    if (std::regex_search(url, match, url_regex))
    {
        std::string host = match[1].str();
        int port = 80;
        if (match[2].matched)
        {
            port = std::stoi(match[2].str());
        }
        else if (url.starts_with("https://"))
        {
            port = 443;
        }
        return {host, port};
    }
    return {"127.0.0.1", 4096};
}

TimeInfo parse_time_info(const json& j)
{
    TimeInfo info;
    if (j.contains("created"))
        info.created = j["created"].get<int64_t>();
    if (j.contains("updated"))
        info.updated = j["updated"].get<int64_t>();
    if (j.contains("compacting") && !j["compacting"].is_null())
        info.compacting = j["compacting"].get<int64_t>();
    if (j.contains("archived") && !j["archived"].is_null())
        info.archived = j["archived"].get<int64_t>();
    if (j.contains("completed") && !j["completed"].is_null())
        info.completed = j["completed"].get<int64_t>();
    return info;
}

SessionInfo parse_session(const json& j)
{
    SessionInfo session;
    if (j.contains("id"))
        session.id = j["id"].get<std::string>();
    if (j.contains("slug"))
        session.slug = j["slug"].get<std::string>();
    if (j.contains("projectID"))
        session.project_id = j["projectID"].get<std::string>();
    if (j.contains("directory"))
        session.directory = j["directory"].get<std::string>();
    if (j.contains("title"))
        session.title = j["title"].get<std::string>();
    if (j.contains("version"))
        session.version = j["version"].get<std::string>();
    if (j.contains("time"))
        session.time = parse_time_info(j["time"]);
    if (j.contains("parentID") && !j["parentID"].is_null())
        session.parent_id = j["parentID"].get<std::string>();
    if (j.contains("shareURL") && !j["shareURL"].is_null())
        session.share_url = j["shareURL"].get<std::string>();
    return session;
}

Project parse_project(const json& j)
{
    Project project;
    if (j.contains("id"))
        project.id = j["id"].get<std::string>();
    if (j.contains("worktree"))
        project.worktree = j["worktree"].get<std::string>();
    if (j.contains("vcs") && !j["vcs"].is_null())
        project.vcs = j["vcs"].get<std::string>();
    if (j.contains("name") && !j["name"].is_null())
        project.name = j["name"].get<std::string>();
    if (j.contains("time"))
        project.time = parse_time_info(j["time"]);
    if (j.contains("sandboxes"))
    {
        for (const auto& s : j["sandboxes"])
        {
            project.sandboxes.push_back(s.get<std::string>());
        }
    }
    return project;
}

PermissionRequest parse_permission_request(const json& j)
{
    PermissionRequest req;
    if (j.contains("id"))
        req.id = j["id"].get<std::string>();
    if (j.contains("sessionID"))
        req.session_id = j["sessionID"].get<std::string>();
    if (j.contains("permission"))
        req.permission = j["permission"].get<std::string>();
    if (j.contains("patterns"))
    {
        for (const auto& p : j["patterns"])
        {
            req.patterns.push_back(p.get<std::string>());
        }
    }
    if (j.contains("time"))
        req.time = parse_time_info(j["time"]);
    return req;
}

// File operations parsing

FileEntry parse_file_entry(const json& j)
{
    FileEntry entry;
    if (j.contains("name"))
        entry.name = j["name"].get<std::string>();
    if (j.contains("path"))
        entry.path = j["path"].get<std::string>();
    if (j.contains("isDirectory"))
        entry.is_directory = j["isDirectory"].get<bool>();
    if (j.contains("size") && !j["size"].is_null())
        entry.size = j["size"].get<int64_t>();
    if (j.contains("modified") && !j["modified"].is_null())
        entry.modified = j["modified"].get<int64_t>();
    return entry;
}

FileContent parse_file_content(const json& j)
{
    FileContent content;
    if (j.contains("path"))
        content.path = j["path"].get<std::string>();
    if (j.contains("content"))
        content.content = j["content"].get<std::string>();
    if (j.contains("encoding") && !j["encoding"].is_null())
        content.encoding = j["encoding"].get<std::string>();
    return content;
}

FileStatus parse_file_status(const json& j)
{
    FileStatus status;
    if (j.contains("path"))
        status.path = j["path"].get<std::string>();
    if (j.contains("status"))
        status.status = j["status"].get<std::string>();
    if (j.contains("additions") && !j["additions"].is_null())
        status.additions = j["additions"].get<int>();
    if (j.contains("deletions") && !j["deletions"].is_null())
        status.deletions = j["deletions"].get<int>();
    return status;
}

// Find operations parsing

TextMatch parse_text_match(const json& j)
{
    TextMatch match;
    if (j.contains("path"))
        match.path = j["path"].get<std::string>();
    if (j.contains("line"))
        match.line = j["line"].get<int>();
    if (j.contains("column"))
        match.column = j["column"].get<int>();
    if (j.contains("text"))
        match.text = j["text"].get<std::string>();
    if (j.contains("match"))
        match.match = j["match"].get<std::string>();
    return match;
}

TextSearchResult parse_text_search_result(const json& j)
{
    TextSearchResult result;
    if (j.contains("matches") && j["matches"].is_array())
    {
        for (const auto& m : j["matches"])
            result.matches.push_back(parse_text_match(m));
    }
    if (j.contains("totalMatches"))
        result.total_matches = j["totalMatches"].get<int>();
    if (j.contains("truncated"))
        result.truncated = j["truncated"].get<bool>();
    return result;
}

FileMatch parse_file_match(const json& j)
{
    FileMatch match;
    if (j.contains("path"))
        match.path = j["path"].get<std::string>();
    if (j.contains("name"))
        match.name = j["name"].get<std::string>();
    if (j.contains("isDirectory"))
        match.is_directory = j["isDirectory"].get<bool>();
    return match;
}

SymbolMatch parse_symbol_match(const json& j)
{
    SymbolMatch match;
    if (j.contains("name"))
        match.name = j["name"].get<std::string>();
    if (j.contains("kind"))
        match.kind = j["kind"].get<std::string>();
    if (j.contains("path"))
        match.path = j["path"].get<std::string>();
    if (j.contains("line"))
        match.line = j["line"].get<int>();
    if (j.contains("column"))
        match.column = j["column"].get<int>();
    if (j.contains("container") && !j["container"].is_null())
        match.container = j["container"].get<std::string>();
    return match;
}

// App info parsing

ModelDetails parse_model_details(const json& j)
{
    ModelDetails model;
    if (j.contains("id"))
        model.id = j["id"].get<std::string>();
    if (j.contains("name"))
        model.name = j["name"].get<std::string>();
    if (j.contains("description") && !j["description"].is_null())
        model.description = j["description"].get<std::string>();
    if (j.contains("contextLength") && !j["contextLength"].is_null())
        model.context_length = j["contextLength"].get<int>();
    if (j.contains("inputCost") && !j["inputCost"].is_null())
        model.input_cost = j["inputCost"].get<double>();
    if (j.contains("outputCost") && !j["outputCost"].is_null())
        model.output_cost = j["outputCost"].get<double>();
    return model;
}

ProviderDetails parse_provider_details(const json& j)
{
    ProviderDetails provider;
    if (j.contains("id"))
        provider.id = j["id"].get<std::string>();
    if (j.contains("name"))
        provider.name = j["name"].get<std::string>();
    if (j.contains("models") && j["models"].is_array())
    {
        for (const auto& m : j["models"])
            provider.models.push_back(parse_model_details(m));
    }
    if (j.contains("configured"))
        provider.configured = j["configured"].get<bool>();
    if (j.contains("error") && !j["error"].is_null())
        provider.error = j["error"].get<std::string>();
    return provider;
}

ModeInfo parse_mode_info(const json& j)
{
    ModeInfo mode;
    if (j.contains("id"))
        mode.id = j["id"].get<std::string>();
    if (j.contains("name"))
        mode.name = j["name"].get<std::string>();
    if (j.contains("description") && !j["description"].is_null())
        mode.description = j["description"].get<std::string>();
    return mode;
}

AgentInfo parse_agent_info(const json& j)
{
    AgentInfo agent;
    if (j.contains("id"))
        agent.id = j["id"].get<std::string>();
    if (j.contains("name"))
        agent.name = j["name"].get<std::string>();
    if (j.contains("description") && !j["description"].is_null())
        agent.description = j["description"].get<std::string>();
    return agent;
}

SkillInfo parse_skill_info(const json& j)
{
    SkillInfo skill;
    if (j.contains("id"))
        skill.id = j["id"].get<std::string>();
    if (j.contains("name"))
        skill.name = j["name"].get<std::string>();
    if (j.contains("description") && !j["description"].is_null())
        skill.description = j["description"].get<std::string>();
    if (j.contains("commands") && j["commands"].is_array())
    {
        for (const auto& c : j["commands"])
            skill.commands.push_back(c.get<std::string>());
    }
    return skill;
}

// Config parsing

ConfigProvider parse_config_provider(const json& j)
{
    ConfigProvider provider;
    if (j.contains("id"))
        provider.id = j["id"].get<std::string>();
    if (j.contains("enabled"))
        provider.enabled = j["enabled"].get<bool>();
    if (j.contains("apiKeyEnv") && !j["apiKeyEnv"].is_null())
        provider.api_key_env = j["apiKeyEnv"].get<std::string>();
    if (j.contains("hasKey"))
        provider.has_key = j["hasKey"].get<bool>();
    return provider;
}

Config parse_config(const json& j)
{
    Config config;
    if (j.contains("defaultProvider") && !j["defaultProvider"].is_null())
        config.default_provider = j["defaultProvider"].get<std::string>();
    if (j.contains("defaultModel") && !j["defaultModel"].is_null())
        config.default_model = j["defaultModel"].get<std::string>();
    if (j.contains("autoApprove") && !j["autoApprove"].is_null())
        config.auto_approve = j["autoApprove"].get<bool>();
    if (j.contains("maxTokens") && !j["maxTokens"].is_null())
        config.max_tokens = j["maxTokens"].get<int>();
    if (j.contains("temperature") && !j["temperature"].is_null())
        config.temperature = j["temperature"].get<double>();
    if (j.contains("theme") && !j["theme"].is_null())
        config.theme = j["theme"].get<std::string>();
    if (j.contains("showCost") && !j["showCost"].is_null())
        config.show_cost = j["showCost"].get<bool>();
    if (j.contains("providers") && j["providers"].is_array())
    {
        for (const auto& p : j["providers"])
            config.providers.push_back(parse_config_provider(p));
    }
    return config;
}

std::string log_level_to_string(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug: return "debug";
    case LogLevel::Info: return "info";
    case LogLevel::Warn: return "warn";
    case LogLevel::Error: return "error";
    default: return "info";
    }
}

// MCP parsing

McpTool parse_mcp_tool(const json& j)
{
    McpTool tool;
    if (j.contains("name"))
        tool.name = j["name"].get<std::string>();
    if (j.contains("description") && !j["description"].is_null())
        tool.description = j["description"].get<std::string>();
    return tool;
}

McpResource parse_mcp_resource(const json& j)
{
    McpResource resource;
    if (j.contains("uri"))
        resource.uri = j["uri"].get<std::string>();
    if (j.contains("name"))
        resource.name = j["name"].get<std::string>();
    if (j.contains("description") && !j["description"].is_null())
        resource.description = j["description"].get<std::string>();
    if (j.contains("mimeType") && !j["mimeType"].is_null())
        resource.mime_type = j["mimeType"].get<std::string>();
    return resource;
}

McpServer parse_mcp_server(const json& j)
{
    McpServer server;
    if (j.contains("id"))
        server.id = j["id"].get<std::string>();
    if (j.contains("name"))
        server.name = j["name"].get<std::string>();
    if (j.contains("status"))
        server.status = j["status"].get<std::string>();
    if (j.contains("error") && !j["error"].is_null())
        server.error = j["error"].get<std::string>();
    if (j.contains("tools") && j["tools"].is_array())
    {
        for (const auto& t : j["tools"])
            server.tools.push_back(parse_mcp_tool(t));
    }
    if (j.contains("resources") && j["resources"].is_array())
    {
        for (const auto& r : j["resources"])
            server.resources.push_back(parse_mcp_resource(r));
    }
    return server;
}

McpStatus parse_mcp_status(const json& j)
{
    McpStatus status;
    if (j.contains("servers") && j["servers"].is_array())
    {
        for (const auto& s : j["servers"])
            status.servers.push_back(parse_mcp_server(s));
    }
    else if (j.is_array())
    {
        // Sometimes returns array directly
        for (const auto& s : j)
            status.servers.push_back(parse_mcp_server(s));
    }
    return status;
}

// Question parsing

QuestionOption parse_question_option(const json& j)
{
    QuestionOption opt;
    if (j.contains("label"))
        opt.label = j["label"].get<std::string>();
    if (j.contains("value"))
        opt.value = j["value"].get<std::string>();
    if (j.contains("description") && !j["description"].is_null())
        opt.description = j["description"].get<std::string>();
    return opt;
}

Question parse_question(const json& j)
{
    Question q;
    if (j.contains("id"))
        q.id = j["id"].get<std::string>();
    if (j.contains("sessionID"))
        q.session_id = j["sessionID"].get<std::string>();
    if (j.contains("text"))
        q.text = j["text"].get<std::string>();
    if (j.contains("type"))
        q.type = j["type"].get<std::string>();
    if (j.contains("options") && j["options"].is_array())
    {
        for (const auto& o : j["options"])
            q.options.push_back(parse_question_option(o));
    }
    if (j.contains("defaultValue") && !j["defaultValue"].is_null())
        q.default_value = j["defaultValue"].get<std::string>();
    if (j.contains("time"))
        q.time = parse_time_info(j["time"]);
    return q;
}

// Worktree parsing

Worktree parse_worktree(const json& j)
{
    Worktree wt;
    if (j.contains("id"))
        wt.id = j["id"].get<std::string>();
    if (j.contains("path"))
        wt.path = j["path"].get<std::string>();
    if (j.contains("branch"))
        wt.branch = j["branch"].get<std::string>();
    if (j.contains("isMain"))
        wt.is_main = j["isMain"].get<bool>();
    if (j.contains("commit") && !j["commit"].is_null())
        wt.commit = j["commit"].get<std::string>();
    if (j.contains("isBare") && !j["isBare"].is_null())
        wt.is_bare = j["isBare"].get<bool>();
    if (j.contains("isDetached") && !j["isDetached"].is_null())
        wt.is_detached = j["isDetached"].get<bool>();
    if (j.contains("time"))
        wt.time = parse_time_info(j["time"]);
    return wt;
}

// Tool parsing

ToolParameter parse_tool_parameter(const json& j)
{
    ToolParameter param;
    if (j.contains("name"))
        param.name = j["name"].get<std::string>();
    if (j.contains("type"))
        param.type = j["type"].get<std::string>();
    if (j.contains("description") && !j["description"].is_null())
        param.description = j["description"].get<std::string>();
    if (j.contains("required"))
        param.required = j["required"].get<bool>();
    if (j.contains("default") && !j["default"].is_null())
        param.default_value = j["default"].dump();
    return param;
}

ToolInfo parse_tool_info(const json& j)
{
    ToolInfo tool;
    if (j.contains("id"))
        tool.id = j["id"].get<std::string>();
    if (j.contains("name"))
        tool.name = j["name"].get<std::string>();
    if (j.contains("description") && !j["description"].is_null())
        tool.description = j["description"].get<std::string>();
    if (j.contains("parameters") && j["parameters"].is_array())
    {
        for (const auto& p : j["parameters"])
            tool.parameters.push_back(parse_tool_parameter(p));
    }
    if (j.contains("category") && !j["category"].is_null())
        tool.category = j["category"].get<std::string>();
    if (j.contains("enabled"))
        tool.enabled = j["enabled"].get<bool>();
    return tool;
}

// LSP parsing

LspServer parse_lsp_server(const json& j)
{
    LspServer server;
    if (j.contains("language"))
        server.language = j["language"].get<std::string>();
    if (j.contains("name"))
        server.name = j["name"].get<std::string>();
    if (j.contains("status"))
        server.status = j["status"].get<std::string>();
    if (j.contains("version") && !j["version"].is_null())
        server.version = j["version"].get<std::string>();
    if (j.contains("error") && !j["error"].is_null())
        server.error = j["error"].get<std::string>();
    if (j.contains("pid") && !j["pid"].is_null())
        server.pid = j["pid"].get<int>();
    return server;
}

LspStatus parse_lsp_status(const json& j)
{
    LspStatus status;
    if (j.contains("servers") && j["servers"].is_array())
    {
        for (const auto& s : j["servers"])
            status.servers.push_back(parse_lsp_server(s));
    }
    else if (j.is_array())
    {
        for (const auto& s : j)
            status.servers.push_back(parse_lsp_server(s));
    }
    return status;
}

// Formatter parsing

Formatter parse_formatter(const json& j)
{
    Formatter fmt;
    if (j.contains("language"))
        fmt.language = j["language"].get<std::string>();
    if (j.contains("name"))
        fmt.name = j["name"].get<std::string>();
    if (j.contains("status"))
        fmt.status = j["status"].get<std::string>();
    if (j.contains("version") && !j["version"].is_null())
        fmt.version = j["version"].get<std::string>();
    if (j.contains("error") && !j["error"].is_null())
        fmt.error = j["error"].get<std::string>();
    return fmt;
}

FormatterStatus parse_formatter_status(const json& j)
{
    FormatterStatus status;
    if (j.contains("formatters") && j["formatters"].is_array())
    {
        for (const auto& f : j["formatters"])
            status.formatters.push_back(parse_formatter(f));
    }
    else if (j.is_array())
    {
        for (const auto& f : j)
            status.formatters.push_back(parse_formatter(f));
    }
    return status;
}

// Auth parsing

AuthResult parse_auth_result(const json& j)
{
    AuthResult result;
    if (j.contains("success"))
        result.success = j["success"].get<bool>();
    if (j.contains("error") && !j["error"].is_null())
        result.error = j["error"].get<std::string>();
    return result;
}

// TUI parsing

TuiStatus parse_tui_status(const json& j)
{
    TuiStatus status;
    if (j.contains("open"))
        status.open = j["open"].get<bool>();
    if (j.contains("focused"))
        status.focused = j["focused"].get<bool>();
    if (j.contains("size") && j["size"].is_object())
    {
        const auto& s = j["size"];
        if (s.contains("width"))
            status.size.width = s["width"].get<int>();
        if (s.contains("height"))
            status.size.height = s["height"].get<int>();
    }
    if (j.contains("selection") && j["selection"].is_object())
    {
        TuiSelection sel;
        const auto& s = j["selection"];
        if (s.contains("start") && s["start"].is_object())
        {
            sel.start.x = s["start"].value("x", 0);
            sel.start.y = s["start"].value("y", 0);
        }
        if (s.contains("end") && s["end"].is_object())
        {
            sel.end.x = s["end"].value("x", 0);
            sel.end.y = s["end"].value("y", 0);
        }
        status.selection = sel;
    }
    return status;
}

TuiRender parse_tui_render(const json& j)
{
    TuiRender render;
    if (j.contains("lines") && j["lines"].is_array())
    {
        for (const auto& line : j["lines"])
            render.lines.push_back(line.get<std::string>());
    }
    if (j.contains("size") && j["size"].is_object())
    {
        render.size.width = j["size"].value("width", 0);
        render.size.height = j["size"].value("height", 0);
    }
    return render;
}

PtySession parse_pty_session(const json& j)
{
    PtySession session;
    session.id = j.value("id", "");
    session.shell = j.value("shell", "");
    session.pid = j.value("pid", 0);
    session.cols = j.value("cols", 80);
    session.rows = j.value("rows", 24);
    session.status = j.value("status", "");
    if (j.contains("exitCode") && !j["exitCode"].is_null())
        session.exit_code = j["exitCode"].get<int>();
    if (j.contains("exit_code") && !j["exit_code"].is_null())
        session.exit_code = j["exit_code"].get<int>();
    if (j.contains("time") && j["time"].is_object())
        session.time = parse_time_info(j["time"]);
    return session;
}

Part parse_part(const json& j)
{
    std::string type = j.value("type", "text");

    if (type == "text")
    {
        TextPart part;
        part.type = type;
        if (j.contains("id"))
            part.id = j["id"].get<std::string>();
        if (j.contains("text"))
            part.text = j["text"].get<std::string>();
        return part;
    }
    else if (type == "file")
    {
        FilePart part;
        part.type = type;
        if (j.contains("id"))
            part.id = j["id"].get<std::string>();
        if (j.contains("file"))
            part.file = j["file"].get<std::string>();
        if (j.contains("content") && !j["content"].is_null())
            part.content = j["content"].get<std::string>();
        return part;
    }
    else if (type == "tool")
    {
        ToolPart part;
        part.type = type;
        if (j.contains("id"))
            part.id = j["id"].get<std::string>();
        if (j.contains("tool"))
            part.tool = j["tool"].get<std::string>();
        if (j.contains("input") && j["input"].is_object())
        {
            for (auto& [key, value] : j["input"].items())
            {
                if (value.is_string())
                    part.input[key] = value.get<std::string>();
                else
                    part.input[key] = value.dump();
            }
        }
        if (j.contains("state") && j["state"].is_object())
        {
            ToolState state;
            state.status = j["state"].value("status", "");
            if (j["state"].contains("error") && !j["state"]["error"].is_null())
                state.error = j["state"]["error"].get<std::string>();
            part.state = state;
        }
        return part;
    }
    else if (type == "reasoning")
    {
        ReasoningPart part;
        part.type = type;
        if (j.contains("id"))
            part.id = j["id"].get<std::string>();
        if (j.contains("text"))
            part.text = j["text"].get<std::string>();
        return part;
    }

    // Default to text part
    TextPart part;
    return part;
}

Message parse_message(const json& j)
{
    std::string role = j.value("role", "user");

    if (role == "assistant")
    {
        AssistantMessage msg;
        if (j.contains("id"))
            msg.id = j["id"].get<std::string>();
        if (j.contains("sessionID"))
            msg.session_id = j["sessionID"].get<std::string>();
        msg.role = role;
        if (j.contains("time"))
            msg.time = parse_time_info(j["time"]);
        if (j.contains("parentID"))
            msg.parent_id = j["parentID"].get<std::string>();
        if (j.contains("modelID"))
            msg.model_id = j["modelID"].get<std::string>();
        if (j.contains("providerID"))
            msg.provider_id = j["providerID"].get<std::string>();
        if (j.contains("mode"))
            msg.mode = j["mode"].get<std::string>();
        if (j.contains("agent"))
            msg.agent = j["agent"].get<std::string>();
        if (j.contains("path"))
        {
            if (j["path"].contains("cwd"))
                msg.path.cwd = j["path"]["cwd"].get<std::string>();
            if (j["path"].contains("root"))
                msg.path.root = j["path"]["root"].get<std::string>();
        }
        if (j.contains("cost"))
            msg.cost = j["cost"].get<double>();
        if (j.contains("tokens") && j["tokens"].is_object())
        {
            auto& t = j["tokens"];
            if (t.contains("input"))
                msg.tokens.input = t["input"].get<int>();
            if (t.contains("output"))
                msg.tokens.output = t["output"].get<int>();
            if (t.contains("reasoning"))
                msg.tokens.reasoning = t["reasoning"].get<int>();
            if (t.contains("cache") && t["cache"].is_object())
            {
                if (t["cache"].contains("read"))
                    msg.tokens.cache.read = t["cache"]["read"].get<int>();
                if (t["cache"].contains("write"))
                    msg.tokens.cache.write = t["cache"]["write"].get<int>();
            }
        }
        if (j.contains("finish") && !j["finish"].is_null())
            msg.finish = j["finish"].get<std::string>();
        return msg;
    }
    else
    {
        UserMessage msg;
        if (j.contains("id"))
            msg.id = j["id"].get<std::string>();
        if (j.contains("sessionID"))
            msg.session_id = j["sessionID"].get<std::string>();
        msg.role = role;
        if (j.contains("time"))
            msg.time = parse_time_info(j["time"]);
        if (j.contains("agent"))
            msg.agent = j["agent"].get<std::string>();
        if (j.contains("model") && j["model"].is_object())
        {
            if (j["model"].contains("providerID"))
                msg.model.provider_id = j["model"]["providerID"].get<std::string>();
            if (j["model"].contains("modelID"))
                msg.model.model_id = j["model"]["modelID"].get<std::string>();
        }
        if (j.contains("system") && !j["system"].is_null())
            msg.system = j["system"].get<std::string>();
        return msg;
    }
}

MessageWithParts parse_message_with_parts(const json& j)
{
    MessageWithParts msg;
    if (j.contains("info"))
        msg.info = parse_message(j["info"]);
    if (j.contains("parts") && j["parts"].is_array())
    {
        for (const auto& p : j["parts"])
        {
            msg.parts.push_back(parse_part(p));
        }
    }
    return msg;
}

} // anonymous namespace

// =============================================================================
// MessageStream Implementation
// =============================================================================

struct MessageStream::Impl
{
    std::mutex mutex;
    std::condition_variable cv;
    std::queue<MessageWithParts> messages;
    bool closed = false;
    std::string error;
};

MessageStream::MessageStream() : impl_(std::make_shared<Impl>()) {}
MessageStream::~MessageStream() = default;
MessageStream::MessageStream(MessageStream&&) noexcept = default;
MessageStream& MessageStream::operator=(MessageStream&&) noexcept = default;

MessageStream::Iterator MessageStream::begin()
{
    return Iterator(this);
}

MessageStream::Iterator MessageStream::end()
{
    return Iterator();
}

std::optional<MessageWithParts> MessageStream::next()
{
    std::unique_lock<std::mutex> lock(impl_->mutex);
    impl_->cv.wait(lock, [this]
                   { return !impl_->messages.empty() || impl_->closed; });

    if (impl_->messages.empty())
    {
        return std::nullopt;
    }

    auto msg = std::move(impl_->messages.front());
    impl_->messages.pop();
    return msg;
}

void MessageStream::close()
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->closed = true;
    impl_->cv.notify_all();
}

MessageStream::Iterator::Iterator() : stream_(nullptr), at_end_(true) {}

MessageStream::Iterator::Iterator(MessageStream* stream) : stream_(stream), at_end_(false)
{
    fetch_next();
}

MessageStream::Iterator::reference MessageStream::Iterator::operator*() const
{
    return *current_;
}

MessageStream::Iterator::pointer MessageStream::Iterator::operator->() const
{
    return &*current_;
}

MessageStream::Iterator& MessageStream::Iterator::operator++()
{
    fetch_next();
    return *this;
}

bool MessageStream::Iterator::operator==(const Iterator& other) const
{
    return at_end_ == other.at_end_;
}

bool MessageStream::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

void MessageStream::Iterator::fetch_next()
{
    if (stream_)
    {
        current_ = stream_->next();
        if (!current_)
        {
            at_end_ = true;
        }
    }
}

// =============================================================================
// EventStream Implementation
// =============================================================================

struct EventStream::Impl
{
    std::mutex mutex;
    std::condition_variable cv;
    std::queue<Event> events;
    bool closed = false;
    std::string error;
    std::unique_ptr<Transport> transport;
};

EventStream::EventStream() : impl_(std::make_shared<Impl>()) {}
EventStream::~EventStream()
{
    close();
}
EventStream::EventStream(EventStream&&) noexcept = default;
EventStream& EventStream::operator=(EventStream&&) noexcept = default;

EventStream::Iterator EventStream::begin()
{
    return Iterator(this);
}

EventStream::Iterator EventStream::end()
{
    return Iterator();
}

std::optional<Event> EventStream::next()
{
    std::unique_lock<std::mutex> lock(impl_->mutex);
    impl_->cv.wait(lock, [this]
                   { return !impl_->events.empty() || impl_->closed; });

    if (impl_->events.empty())
    {
        return std::nullopt;
    }

    auto event = std::move(impl_->events.front());
    impl_->events.pop();
    return event;
}

void EventStream::close()
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->closed = true;
    if (impl_->transport)
    {
        impl_->transport->stop_sse();
    }
    impl_->cv.notify_all();
}

EventStream::Iterator::Iterator() : stream_(nullptr), at_end_(true) {}

EventStream::Iterator::Iterator(EventStream* stream) : stream_(stream), at_end_(false)
{
    fetch_next();
}

EventStream::Iterator::reference EventStream::Iterator::operator*() const
{
    return *current_;
}

EventStream::Iterator::pointer EventStream::Iterator::operator->() const
{
    return &*current_;
}

EventStream::Iterator& EventStream::Iterator::operator++()
{
    fetch_next();
    return *this;
}

bool EventStream::Iterator::operator==(const Iterator& other) const
{
    return at_end_ == other.at_end_;
}

bool EventStream::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

void EventStream::Iterator::fetch_next()
{
    if (stream_)
    {
        current_ = stream_->next();
        if (!current_)
        {
            at_end_ = true;
        }
    }
}

// =============================================================================
// Client Implementation
// =============================================================================

struct Client::Impl
{
    ClientOptions opts;
    std::unique_ptr<Transport> transport;
    std::unique_ptr<Server> server;  // Owned server if we spawned it
    std::string server_url;
    bool connected = false;

    Impl(ClientOptions options)
        : opts(std::move(options))
    {
    }

    HttpResponse request(const std::string& method, const std::string& path, const std::string& body = {})
    {
        HttpRequest req;
        req.method = method;
        req.path = path;
        req.body = body;
        req.content_type = "application/json";
        return transport->request(req);
    }

    bool try_connect(const std::string& host, int port)
    {
        try
        {
            auto test_transport = std::make_unique<HttpTransport>(host, port);
            test_transport->set_connection_timeout(2);
            test_transport->set_read_timeout(2);

            HttpRequest req;
            req.method = "GET";
            req.path = "/global/health";
            auto response = test_transport->request(req);

            if (response.status == 200)
            {
                // Success - use this transport
                if (opts.basic_auth)
                {
                    transport = std::make_unique<HttpTransport>(
                        host, port,
                        opts.basic_auth->first,
                        opts.basic_auth->second);
                }
                else
                {
                    transport = std::make_unique<HttpTransport>(host, port);
                }

                if (opts.directory)
                    static_cast<HttpTransport*>(transport.get())->set_directory(*opts.directory);

                static_cast<HttpTransport*>(transport.get())->set_connection_timeout(opts.connection_timeout);
                static_cast<HttpTransport*>(transport.get())->set_read_timeout(opts.read_timeout);

                server_url = "http://" + host + ":" + std::to_string(port);
                connected = true;
                return true;
            }
        }
        catch (...)
        {
            // Connection failed
        }
        return false;
    }
};

Client::Client() : Client(ClientOptions{})
{
}

Client::Client(ClientOptions opts)
    : impl_(std::make_unique<Impl>(std::move(opts)))
{
    connect();
}

Client::Client(ClientOptions opts, std::unique_ptr<Transport> transport)
    : impl_(std::make_unique<Impl>(std::move(opts)))
{
    impl_->transport = std::move(transport);
    impl_->connected = true;
    impl_->server_url = impl_->opts.base_url.value_or("http://127.0.0.1:4096");
}

void Client::connect()
{
    // If explicit URL provided, connect to it (no spawn)
    if (impl_->opts.base_url)
    {
        auto [host, port] = parse_url(*impl_->opts.base_url);
        if (impl_->try_connect(host, port))
            return;

        throw std::runtime_error("Cannot connect to server at " + *impl_->opts.base_url);
    }

    // Spawn dedicated server with OS-assigned port
    ServerOptions server_opts;
    server_opts.port = 0;  // OS assigns port
    server_opts.opencode_binary = impl_->opts.opencode_path;
    server_opts.startup_timeout = std::chrono::milliseconds(impl_->opts.startup_timeout_ms);
    if (impl_->opts.directory)
        server_opts.working_directory = *impl_->opts.directory;

    impl_->server = std::make_unique<Server>(Server::spawn(server_opts));

    // Connect to the spawned server's actual URL
    auto [host, port] = parse_url(impl_->server->url());
    if (impl_->try_connect(host, port))
        return;

    throw std::runtime_error("Failed to connect to spawned server at " + impl_->server->url());
}

Client::~Client() = default;
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

// =============================================================================
// Connection
// =============================================================================

bool Client::is_connected() const
{
    return impl_->connected;
}

std::string Client::server_url() const
{
    return impl_->server_url;
}

const ClientOptions& Client::options() const
{
    return impl_->opts;
}

// =============================================================================
// Health
// =============================================================================

HealthInfo Client::health()
{
    auto response = impl_->request("GET", "/global/health");
    if (response.status != 200)
    {
        throw std::runtime_error("Health check failed: " + response.error);
    }

    auto j = json::parse(response.body);
    HealthInfo info;
    if (j.contains("healthy"))
        info.healthy = j["healthy"].get<bool>();
    if (j.contains("version"))
        info.version = j["version"].get<std::string>();
    return info;
}

// =============================================================================
// Sessions
// =============================================================================

std::vector<SessionInfo> Client::list_sessions()
{
    auto response = impl_->request("GET", "/session");
    if (response.status != 200)
    {
        throw std::runtime_error("List sessions failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<SessionInfo> sessions;
    if (j.is_array())
    {
        for (const auto& item : j)
        {
            sessions.push_back(parse_session(item));
        }
    }
    return sessions;
}

Session Client::create_session(const std::string& title)
{
    json body = json::object();
    if (!title.empty())
    {
        body["title"] = title;
    }

    auto response = impl_->request("POST", "/session", body.dump());
    if (response.status != 200)
    {
        throw std::runtime_error("Create session failed: " + response.error);
    }

    auto info = parse_session(json::parse(response.body));
    return Session(this, std::move(info));
}

Session Client::get_session(const std::string& session_id)
{
    auto response = impl_->request("GET", "/session/" + session_id);
    if (response.status == 404)
    {
        throw std::runtime_error("Session not found: " + session_id);
    }
    if (response.status != 200)
    {
        throw std::runtime_error("Get session failed: " + response.error);
    }

    auto info = parse_session(json::parse(response.body));
    return Session(this, std::move(info));
}

bool Client::delete_session(const std::string& session_id)
{
    auto response = impl_->request("DELETE", "/session/" + session_id);
    return response.status == 200;
}

// =============================================================================
// Messages
// =============================================================================

MessageWithParts Client::send_message(
    const std::string& session_id,
    const std::string& prompt,
    const std::string& provider_id,
    const std::string& model_id)
{
    json body = json::object();
    body["parts"] = json::array({json::object({{"type", "text"}, {"text", prompt}})});

    // Add model specification if provided
    if (!provider_id.empty() || !model_id.empty())
    {
        json model = json::object();
        if (!provider_id.empty())
            model["providerID"] = provider_id;
        if (!model_id.empty())
            model["modelID"] = model_id;
        body["model"] = model;
    }

    auto response = impl_->request("POST", "/session/" + session_id + "/message", body.dump());
    if (response.status != 200)
    {
        throw std::runtime_error("Send message failed: " + response.error);
    }

    return parse_message_with_parts(json::parse(response.body));
}

void Client::send_message_streaming(
    const std::string& session_id,
    const std::string& prompt,
    const std::string& provider_id,
    const std::string& model_id,
    StreamOptions options)
{
    // Create SSE transport for events
    auto [host, port] = parse_url(impl_->server_url);
    std::unique_ptr<HttpTransport> sse_transport;

    if (impl_->opts.basic_auth)
    {
        sse_transport = std::make_unique<HttpTransport>(
            host, port,
            impl_->opts.basic_auth->first,
            impl_->opts.basic_auth->second);
    }
    else
    {
        sse_transport = std::make_unique<HttpTransport>(host, port);
    }

    if (impl_->opts.directory)
    {
        sse_transport->set_directory(*impl_->opts.directory);
    }

    // Shared state for SSE callback
    struct StreamState
    {
        std::string session_id;
        StreamOptions options;
        std::atomic<bool> done{false};
        std::atomic<bool> connected{false};
        std::mutex mutex;
        std::condition_variable cv;
    };
    auto state = std::make_shared<StreamState>();
    state->session_id = session_id;
    state->options = std::move(options);

    // Start SSE in background to receive part updates
    sse_transport->start_sse(
        "/event",
        {},
        [state](const SSEEvent& sse_event)
        {
            if (state->done)
                return;

            try
            {
                // Parse the JSON data to get event type
                auto j = json::parse(sse_event.data);
                std::string event_type = j.value("type", "");

                // Wait for server.connected to signal we're ready
                if (event_type == "server.connected")
                {
                    state->connected = true;
                    state->cv.notify_all();
                    return;
                }

                // Handle message.part.updated events for our session
                if (event_type == "message.part.updated")
                {
                    auto props = j.value("properties", json::object());
                    if (props.contains("part"))
                    {
                        const auto& part_json = props["part"];
                        std::string event_session_id = part_json.value("sessionID", "");
                        if (event_session_id == state->session_id && state->options.on_part)
                        {
                            // If there's a delta, use it to update the text part
                            Part part = parse_part(part_json);
                            if (props.contains("delta") && props["delta"].is_string())
                            {
                                std::string delta = props["delta"].get<std::string>();
                                if (auto* text_part = std::get_if<TextPart>(&part))
                                {
                                    text_part->text = delta;  // Use delta as the text
                                    text_part->is_delta = true;
                                }
                            }
                            state->options.on_part(part);
                        }
                    }
                }
            }
            catch (...)
            {
                // Ignore parse errors
            }
        },
        [state](const std::string& error)
        {
            if (!state->done && state->options.on_error)
            {
                state->options.on_error(error);
            }
            state->connected = true;  // Unblock waiter on error
            state->cv.notify_all();
        },
        [state]()
        {
            // Connection closed
            state->cv.notify_all();
        }
    );

    // Wait for SSE connection (max 2 seconds)
    {
        std::unique_lock<std::mutex> lock(state->mutex);
        state->cv.wait_for(lock, std::chrono::seconds(2), [state]
        {
            return state->connected.load();
        });
    }

    // Send the message (blocks until complete)
    try
    {
        auto result = send_message(session_id, prompt, provider_id, model_id);
        state->done = true;
        sse_transport->stop_sse();

        if (state->options.on_complete)
        {
            state->options.on_complete(result);
        }
    }
    catch (const std::exception& e)
    {
        state->done = true;
        sse_transport->stop_sse();

        if (state->options.on_error)
        {
            state->options.on_error(e.what());
        }
    }
}

std::vector<MessageWithParts> Client::get_messages(const std::string& session_id, std::optional<int> limit)
{
    std::string path = "/session/" + session_id + "/message";
    if (limit)
    {
        path += "?limit=" + std::to_string(*limit);
    }

    auto response = impl_->request("GET", path);
    if (response.status != 200)
    {
        throw std::runtime_error("Get messages failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<MessageWithParts> messages;
    if (j.is_array())
    {
        for (const auto& item : j)
        {
            messages.push_back(parse_message_with_parts(item));
        }
    }
    return messages;
}

// =============================================================================
// Session Control
// =============================================================================

bool Client::abort_session(const std::string& session_id)
{
    auto response = impl_->request("POST", "/session/" + session_id + "/abort");
    return response.status == 200;
}

bool Client::init_session(
    const std::string& session_id,
    const std::string& provider_id,
    const std::string& model_id)
{
    json body = json::object();
    body["provider_id"] = provider_id;
    body["model_id"] = model_id;
    body["message_id"] = "";  // Required by API

    auto response = impl_->request("POST", "/session/" + session_id + "/init", body.dump());
    return response.status == 200;
}

std::string Client::summarize_session(
    const std::string& session_id,
    const std::string& provider_id,
    const std::string& model_id)
{
    json body = json::object();
    body["provider_id"] = provider_id;
    body["model_id"] = model_id;

    auto response = impl_->request("POST", "/session/" + session_id + "/summarize", body.dump());
    if (response.status != 200)
    {
        throw std::runtime_error("Summarize session failed: " + response.error);
    }

    auto j = json::parse(response.body);
    return j.value("summary", "");
}

SessionInfo Client::revert_message(
    const std::string& session_id,
    const std::string& message_id,
    const std::string& part_id)
{
    json body = json::object();
    body["message_id"] = message_id;
    if (!part_id.empty())
    {
        body["part_id"] = part_id;
    }

    auto response = impl_->request("POST", "/session/" + session_id + "/revert", body.dump());
    if (response.status != 200)
    {
        throw std::runtime_error("Revert message failed: " + response.error);
    }

    return parse_session(json::parse(response.body));
}

SessionInfo Client::unrevert_session(const std::string& session_id)
{
    auto response = impl_->request("POST", "/session/" + session_id + "/unrevert");
    if (response.status != 200)
    {
        throw std::runtime_error("Unrevert session failed: " + response.error);
    }

    return parse_session(json::parse(response.body));
}

SessionInfo Client::share_session(const std::string& session_id)
{
    auto response = impl_->request("POST", "/session/" + session_id + "/share");
    if (response.status != 200)
    {
        throw std::runtime_error("Share session failed: " + response.error);
    }

    return parse_session(json::parse(response.body));
}

SessionInfo Client::unshare_session(const std::string& session_id)
{
    auto response = impl_->request("DELETE", "/session/" + session_id + "/share");
    if (response.status != 200)
    {
        throw std::runtime_error("Unshare session failed: " + response.error);
    }

    return parse_session(json::parse(response.body));
}

// =============================================================================
// Permissions
// =============================================================================

std::vector<PermissionRequest> Client::list_permissions()
{
    auto response = impl_->request("GET", "/permission");
    if (response.status != 200)
    {
        throw std::runtime_error("List permissions failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<PermissionRequest> requests;
    if (j.is_array())
    {
        for (const auto& item : j)
        {
            requests.push_back(parse_permission_request(item));
        }
    }
    return requests;
}

bool Client::reply_permission(const PermissionReply& reply)
{
    json body = json::object();
    body["action"] = permission_action_to_string(reply.action);
    if (reply.message)
    {
        body["message"] = *reply.message;
    }

    auto response = impl_->request("POST", "/permission/" + reply.request_id, body.dump());
    return response.status == 200;
}

// =============================================================================
// Projects
// =============================================================================

std::vector<Project> Client::list_projects()
{
    auto response = impl_->request("GET", "/project");
    if (response.status != 200)
    {
        throw std::runtime_error("List projects failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<Project> projects;
    if (j.is_array())
    {
        for (const auto& item : j)
        {
            projects.push_back(parse_project(item));
        }
    }
    return projects;
}

Project Client::current_project()
{
    auto response = impl_->request("GET", "/project/current");
    if (response.status != 200)
    {
        throw std::runtime_error("Get current project failed: " + response.error);
    }

    return parse_project(json::parse(response.body));
}

// =============================================================================
// Events
// =============================================================================

EventStream Client::subscribe_events()
{
    EventStream stream;

    // Create a separate transport for SSE
    auto [host, port] = parse_url(impl_->server_url);
    std::unique_ptr<HttpTransport> sse_transport;

    if (impl_->opts.basic_auth)
    {
        sse_transport = std::make_unique<HttpTransport>(
            host,
            port,
            impl_->opts.basic_auth->first,
            impl_->opts.basic_auth->second);
    }
    else
    {
        sse_transport = std::make_unique<HttpTransport>(host, port);
    }

    if (impl_->opts.directory)
    {
        sse_transport->set_directory(*impl_->opts.directory);
    }

    stream.impl_->transport = std::move(sse_transport);

    // Start SSE connection
    auto impl = stream.impl_;
    stream.impl_->transport->start_sse(
        "/event",
        {},
        [impl](const SSEEvent& sse_event)
        {
            // Parse the event and add to queue
            try
            {
                std::lock_guard<std::mutex> lock(impl->mutex);
                if (impl->closed)
                    return;

                // Parse JSON to get event type from data
                auto j = json::parse(sse_event.data);
                std::string event_type = j.value("type", "");
                auto props = j.value("properties", json::object());

                // Parse based on event type
                if (event_type == "server.connected")
                {
                    impl->events.push(ServerConnectedEvent{});
                }
                else if (event_type == "server.heartbeat")
                {
                    impl->events.push(ServerHeartbeatEvent{});
                }
                else if (event_type == "session.created")
                {
                    SessionCreatedEvent event;
                    event.session = parse_session(props);
                    impl->events.push(event);
                }
                else if (event_type == "session.updated")
                {
                    SessionUpdatedEvent event;
                    event.session = parse_session(props);
                    impl->events.push(event);
                }
                else if (event_type == "permission.asked")
                {
                    PermissionAskedEvent event;
                    event.request = parse_permission_request(props);
                    impl->events.push(event);
                }
                else if (event_type == "message.part.updated")
                {
                    if (props.contains("part"))
                    {
                        MessagePartUpdatedEvent event;
                        const auto& part_json = props["part"];
                        event.session_id = part_json.value("sessionID", "");
                        event.message_id = part_json.value("messageID", "");
                        event.part = parse_part(part_json);
                        impl->events.push(event);
                    }
                }
                // Add more event types as needed...

                impl->cv.notify_all();
            }
            catch (const std::exception& e)
            {
                // Log or handle parse errors
            }
        },
        [impl](const std::string& error)
        {
            std::lock_guard<std::mutex> lock(impl->mutex);
            impl->error = error;
            impl->closed = true;
            impl->cv.notify_all();
        },
        [impl]()
        {
            std::lock_guard<std::mutex> lock(impl->mutex);
            impl->closed = true;
            impl->cv.notify_all();
        });

    return stream;
}

// =============================================================================
// File Operations
// =============================================================================

std::vector<FileEntry> Client::list_files(const std::string& path)
{
    std::string encoded_path = path;
    // Simple URL encoding for path (replace spaces, etc.)
    // For a full implementation, use a proper URL encoder

    auto response = impl_->request("GET", "/file?path=" + encoded_path);
    if (response.status != 200)
    {
        throw std::runtime_error("List files failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<FileEntry> entries;
    if (j.is_array())
    {
        for (const auto& item : j)
            entries.push_back(parse_file_entry(item));
    }
    return entries;
}

FileContent Client::read_file(const std::string& path)
{
    auto response = impl_->request("GET", "/file/" + path);
    if (response.status == 404)
    {
        throw std::runtime_error("File not found: " + path);
    }
    if (response.status != 200)
    {
        throw std::runtime_error("Read file failed: " + response.error);
    }

    return parse_file_content(json::parse(response.body));
}

FileStatus Client::file_status(const std::string& path)
{
    auto response = impl_->request("GET", "/file/" + path + "/status");
    if (response.status == 404)
    {
        throw std::runtime_error("File not found: " + path);
    }
    if (response.status != 200)
    {
        throw std::runtime_error("Get file status failed: " + response.error);
    }

    return parse_file_status(json::parse(response.body));
}

// =============================================================================
// Find Operations
// =============================================================================

TextSearchResult Client::find_text(const TextSearchOptions& options)
{
    json body = json::object();
    body["pattern"] = options.pattern;
    if (options.glob)
        body["glob"] = *options.glob;
    if (options.limit)
        body["limit"] = *options.limit;
    body["regex"] = options.regex;
    body["caseSensitive"] = options.case_sensitive;

    auto response = impl_->request("POST", "/find/text", body.dump());
    if (response.status != 200)
    {
        throw std::runtime_error("Find text failed: " + response.error);
    }

    return parse_text_search_result(json::parse(response.body));
}

std::vector<FileMatch> Client::find_files(const FileSearchOptions& options)
{
    json body = json::object();
    body["pattern"] = options.pattern;
    if (options.limit)
        body["limit"] = *options.limit;

    auto response = impl_->request("POST", "/find/files", body.dump());
    if (response.status != 200)
    {
        throw std::runtime_error("Find files failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<FileMatch> matches;
    if (j.is_array())
    {
        for (const auto& item : j)
            matches.push_back(parse_file_match(item));
    }
    return matches;
}

std::vector<SymbolMatch> Client::find_symbols(const SymbolSearchOptions& options)
{
    json body = json::object();
    body["query"] = options.query;
    if (options.limit)
        body["limit"] = *options.limit;

    auto response = impl_->request("POST", "/find/symbols", body.dump());
    if (response.status != 200)
    {
        throw std::runtime_error("Find symbols failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<SymbolMatch> matches;
    if (j.is_array())
    {
        for (const auto& item : j)
            matches.push_back(parse_symbol_match(item));
    }
    return matches;
}

// =============================================================================
// App Information
// =============================================================================

std::vector<ProviderDetails> Client::list_providers()
{
    auto response = impl_->request("GET", "/app/providers");
    if (response.status != 200)
    {
        throw std::runtime_error("List providers failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<ProviderDetails> providers;
    if (j.is_array())
    {
        for (const auto& item : j)
            providers.push_back(parse_provider_details(item));
    }
    return providers;
}

std::vector<ModeInfo> Client::list_modes()
{
    auto response = impl_->request("GET", "/app/modes");
    if (response.status != 200)
    {
        throw std::runtime_error("List modes failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<ModeInfo> modes;
    if (j.is_array())
    {
        for (const auto& item : j)
            modes.push_back(parse_mode_info(item));
    }
    return modes;
}

std::vector<AgentInfo> Client::list_agents()
{
    auto response = impl_->request("GET", "/app/agents");
    if (response.status != 200)
    {
        throw std::runtime_error("List agents failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<AgentInfo> agents;
    if (j.is_array())
    {
        for (const auto& item : j)
            agents.push_back(parse_agent_info(item));
    }
    return agents;
}

std::vector<SkillInfo> Client::list_skills()
{
    auto response = impl_->request("GET", "/app/skills");
    if (response.status != 200)
    {
        throw std::runtime_error("List skills failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<SkillInfo> skills;
    if (j.is_array())
    {
        for (const auto& item : j)
            skills.push_back(parse_skill_info(item));
    }
    return skills;
}

void Client::log(LogLevel level, const std::string& message)
{
    json body = json::object();
    body["level"] = log_level_to_string(level);
    body["message"] = message;

    impl_->request("POST", "/app/log", body.dump());
    // Fire and forget - don't check response
}

// =============================================================================
// Configuration
// =============================================================================

Config Client::get_config()
{
    auto response = impl_->request("GET", "/config");
    if (response.status != 200)
    {
        throw std::runtime_error("Get config failed: " + response.error);
    }

    return parse_config(json::parse(response.body));
}

Config Client::update_config(const ConfigUpdate& updates)
{
    json body = json::object();
    if (updates.default_provider)
        body["defaultProvider"] = *updates.default_provider;
    if (updates.default_model)
        body["defaultModel"] = *updates.default_model;
    if (updates.auto_approve)
        body["autoApprove"] = *updates.auto_approve;
    if (updates.max_tokens)
        body["maxTokens"] = *updates.max_tokens;
    if (updates.temperature)
        body["temperature"] = *updates.temperature;

    auto response = impl_->request("PATCH", "/config", body.dump());
    if (response.status != 200)
    {
        throw std::runtime_error("Update config failed: " + response.error);
    }

    return parse_config(json::parse(response.body));
}

std::vector<ConfigProvider> Client::list_config_providers()
{
    auto response = impl_->request("GET", "/config/providers");
    if (response.status != 200)
    {
        throw std::runtime_error("List config providers failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<ConfigProvider> providers;
    if (j.is_array())
    {
        for (const auto& item : j)
            providers.push_back(parse_config_provider(item));
    }
    return providers;
}

// =============================================================================
// MCP (Model Context Protocol)
// =============================================================================

McpStatus Client::mcp_status()
{
    auto response = impl_->request("GET", "/mcp/status");
    if (response.status != 200)
    {
        throw std::runtime_error("MCP status failed: " + response.error);
    }

    return parse_mcp_status(json::parse(response.body));
}

McpServer Client::mcp_add(const McpServerConfig& config)
{
    json body = json::object();
    body["name"] = config.name;
    body["command"] = config.command;
    if (!config.args.empty())
        body["args"] = config.args;
    if (!config.env.empty())
        body["env"] = config.env;

    auto response = impl_->request("POST", "/mcp", body.dump());
    if (response.status != 200 && response.status != 201)
    {
        throw std::runtime_error("MCP add failed: " + response.error);
    }

    return parse_mcp_server(json::parse(response.body));
}

McpServer Client::mcp_connect(const std::string& server_id)
{
    auto response = impl_->request("POST", "/mcp/" + server_id + "/connect");
    if (response.status != 200)
    {
        throw std::runtime_error("MCP connect failed: " + response.error);
    }

    return parse_mcp_server(json::parse(response.body));
}

McpServer Client::mcp_disconnect(const std::string& server_id)
{
    auto response = impl_->request("POST", "/mcp/" + server_id + "/disconnect");
    if (response.status != 200)
    {
        throw std::runtime_error("MCP disconnect failed: " + response.error);
    }

    return parse_mcp_server(json::parse(response.body));
}

// =============================================================================
// Questions
// =============================================================================

std::vector<Question> Client::list_questions()
{
    auto response = impl_->request("GET", "/question");
    if (response.status != 200)
    {
        throw std::runtime_error("List questions failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<Question> questions;
    if (j.is_array())
    {
        for (const auto& item : j)
            questions.push_back(parse_question(item));
    }
    return questions;
}

bool Client::reply_question(const QuestionReply& reply)
{
    json body = json::object();
    body["answer"] = reply.answer;

    auto response = impl_->request("POST", "/question/" + reply.question_id, body.dump());
    return response.status == 200;
}

bool Client::reject_question(const std::string& question_id)
{
    auto response = impl_->request("DELETE", "/question/" + question_id);
    return response.status == 200 || response.status == 204;
}

// =============================================================================
// Worktrees
// =============================================================================

std::vector<Worktree> Client::list_worktrees()
{
    auto response = impl_->request("GET", "/worktree");
    if (response.status != 200)
    {
        throw std::runtime_error("List worktrees failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<Worktree> worktrees;
    if (j.is_array())
    {
        for (const auto& item : j)
            worktrees.push_back(parse_worktree(item));
    }
    return worktrees;
}

Worktree Client::create_worktree(const WorktreeCreate& options)
{
    json body = json::object();
    body["branch"] = options.branch;
    if (options.path)
        body["path"] = *options.path;
    if (options.base)
        body["base"] = *options.base;
    body["createBranch"] = options.create_branch;

    auto response = impl_->request("POST", "/worktree", body.dump());
    if (response.status != 200 && response.status != 201)
    {
        throw std::runtime_error("Create worktree failed: " + response.error);
    }

    return parse_worktree(json::parse(response.body));
}

bool Client::remove_worktree(const std::string& worktree_id)
{
    auto response = impl_->request("DELETE", "/worktree/" + worktree_id);
    return response.status == 200 || response.status == 204;
}

Worktree Client::reset_worktree(const std::string& worktree_id)
{
    auto response = impl_->request("POST", "/worktree/" + worktree_id + "/reset");
    if (response.status != 200)
    {
        throw std::runtime_error("Reset worktree failed: " + response.error);
    }

    return parse_worktree(json::parse(response.body));
}

// =============================================================================
// Tools
// =============================================================================

std::vector<std::string> Client::list_tool_ids()
{
    auto response = impl_->request("GET", "/tool/ids");
    if (response.status != 200)
    {
        throw std::runtime_error("List tool IDs failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<std::string> ids;
    if (j.is_array())
    {
        for (const auto& item : j)
            ids.push_back(item.get<std::string>());
    }
    return ids;
}

std::vector<ToolInfo> Client::list_tools()
{
    auto response = impl_->request("GET", "/tool");
    if (response.status != 200)
    {
        throw std::runtime_error("List tools failed: " + response.error);
    }

    auto j = json::parse(response.body);
    std::vector<ToolInfo> tools;
    if (j.is_array())
    {
        for (const auto& item : j)
            tools.push_back(parse_tool_info(item));
    }
    return tools;
}

// =============================================================================
// LSP
// =============================================================================

LspStatus Client::lsp_status()
{
    auto response = impl_->request("GET", "/lsp/status");
    if (response.status != 200)
    {
        throw std::runtime_error("LSP status failed: " + response.error);
    }

    return parse_lsp_status(json::parse(response.body));
}

// =============================================================================
// Formatter
// =============================================================================

FormatterStatus Client::formatter_status()
{
    auto response = impl_->request("GET", "/formatter/status");
    if (response.status != 200)
    {
        throw std::runtime_error("Formatter status failed: " + response.error);
    }

    return parse_formatter_status(json::parse(response.body));
}

// =============================================================================
// Auth
// =============================================================================

AuthResult Client::set_auth(const std::string& provider_id, const AuthCredentials& credentials)
{
    json body = json::object();
    body["apiKey"] = credentials.api_key;
    if (credentials.api_base)
        body["apiBase"] = *credentials.api_base;
    if (credentials.organization)
        body["organization"] = *credentials.organization;

    auto response = impl_->request("POST", "/auth/" + provider_id, body.dump());

    if (response.status == 200)
        return {true, std::nullopt};

    try
    {
        auto j = json::parse(response.body);
        return parse_auth_result(j);
    }
    catch (...)
    {
        return {false, response.error};
    }
}

AuthResult Client::remove_auth(const std::string& provider_id)
{
    auto response = impl_->request("DELETE", "/auth/" + provider_id);

    if (response.status == 200 || response.status == 204)
        return {true, std::nullopt};

    try
    {
        auto j = json::parse(response.body);
        return parse_auth_result(j);
    }
    catch (...)
    {
        return {false, response.error};
    }
}

// =============================================================================
// Parts
// =============================================================================

bool Client::delete_part(
    const std::string& session_id,
    const std::string& message_id,
    const std::string& part_id)
{
    std::string path = "/session/" + session_id + "/message/" + message_id + "/part/" + part_id;
    auto response = impl_->request("DELETE", path);
    return response.status == 200 || response.status == 204;
}

Part Client::update_part(
    const std::string& session_id,
    const std::string& message_id,
    const std::string& part_id,
    const std::string& text)
{
    json body = json::object();
    body["text"] = text;

    std::string path = "/session/" + session_id + "/message/" + message_id + "/part/" + part_id;
    auto response = impl_->request("PATCH", path, body.dump());

    if (response.status != 200)
    {
        throw std::runtime_error("Update part failed: " + response.error);
    }

    return parse_part(json::parse(response.body));
}


// =============================================================================
// TUI Methods
// =============================================================================

void Client::tui_open()
{
    auto response = impl_->request("POST", "/tui/open");
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("TUI open failed: " + response.error);
}

void Client::tui_close()
{
    auto response = impl_->request("POST", "/tui/close");
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("TUI close failed: " + response.error);
}

void Client::tui_focus()
{
    auto response = impl_->request("POST", "/tui/focus");
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("TUI focus failed: " + response.error);
}

void Client::tui_blur()
{
    auto response = impl_->request("POST", "/tui/blur");
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("TUI blur failed: " + response.error);
}

void Client::tui_resize(int width, int height)
{
    json body = json::object();
    body["width"] = width;
    body["height"] = height;
    auto response = impl_->request("POST", "/tui/resize", body.dump());
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("TUI resize failed: " + response.error);
}

void Client::tui_select(const TuiSelection& selection)
{
    json body = json::object();
    body["start"] = {{"x", selection.start.x}, {"y", selection.start.y}};
    body["end"] = {{"x", selection.end.x}, {"y", selection.end.y}};
    auto response = impl_->request("POST", "/tui/select", body.dump());
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("TUI select failed: " + response.error);
}

TuiStatus Client::tui_status()
{
    auto response = impl_->request("GET", "/tui/status");
    if (response.status != 200)
        throw std::runtime_error("TUI status failed: " + response.error);
    return parse_tui_status(json::parse(response.body));
}

void Client::tui_scroll(int lines)
{
    json body = json::object();
    body["lines"] = lines;
    auto response = impl_->request("POST", "/tui/scroll", body.dump());
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("TUI scroll failed: " + response.error);
}

void Client::tui_input(const std::string& text)
{
    json body = json::object();
    body["text"] = text;
    auto response = impl_->request("POST", "/tui/input", body.dump());
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("TUI input failed: " + response.error);
}

void Client::tui_copy()
{
    auto response = impl_->request("POST", "/tui/copy");
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("TUI copy failed: " + response.error);
}

std::string Client::tui_paste()
{
    auto response = impl_->request("POST", "/tui/paste");
    if (response.status != 200)
        throw std::runtime_error("TUI paste failed: " + response.error);
    auto j = json::parse(response.body);
    return j.value("text", "");
}

void Client::tui_clear()
{
    auto response = impl_->request("POST", "/tui/clear");
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("TUI clear failed: " + response.error);
}

TuiRender Client::tui_render()
{
    auto response = impl_->request("GET", "/tui/render");
    if (response.status != 200)
        throw std::runtime_error("TUI render failed: " + response.error);
    return parse_tui_render(json::parse(response.body));
}

// =============================================================================
// PTY Methods
// =============================================================================

std::vector<PtySession> Client::list_pty_sessions()
{
    auto response = impl_->request("GET", "/pty");
    if (response.status != 200)
        throw std::runtime_error("List PTY sessions failed: " + response.error);

    std::vector<PtySession> sessions;
    auto j = json::parse(response.body);
    if (j.is_array())
    {
        for (const auto& item : j)
            sessions.push_back(parse_pty_session(item));
    }
    return sessions;
}

PtySession Client::create_pty(const PtyCreate& options)
{
    json body = json::object();
    if (options.shell)
        body["shell"] = *options.shell;
    if (options.cwd)
        body["cwd"] = *options.cwd;
    if (options.cols)
        body["cols"] = *options.cols;
    if (options.rows)
        body["rows"] = *options.rows;
    if (!options.env.empty())
    {
        json env_obj = json::object();
        for (const auto& [k, v] : options.env)
            env_obj[k] = v;
        body["env"] = env_obj;
    }

    auto response = impl_->request("POST", "/pty", body.dump());
    if (response.status != 200 && response.status != 201)
        throw std::runtime_error("Create PTY failed: " + response.error);
    return parse_pty_session(json::parse(response.body));
}

void Client::pty_write(const std::string& pty_id, const std::string& data)
{
    json body = json::object();
    body["data"] = data;
    auto response = impl_->request("POST", "/pty/" + pty_id + "/write", body.dump());
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("PTY write failed: " + response.error);
}

PtySession Client::pty_resize(const std::string& pty_id, int cols, int rows)
{
    json body = json::object();
    body["cols"] = cols;
    body["rows"] = rows;
    auto response = impl_->request("POST", "/pty/" + pty_id + "/resize", body.dump());
    if (response.status != 200)
        throw std::runtime_error("PTY resize failed: " + response.error);
    return parse_pty_session(json::parse(response.body));
}

bool Client::close_pty(const std::string& pty_id)
{
    auto response = impl_->request("DELETE", "/pty/" + pty_id);
    if (response.status == 404)
        return false;
    if (response.status != 200 && response.status != 204)
        throw std::runtime_error("Close PTY failed: " + response.error);
    return true;
}

} // namespace opencode
