// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#pragma once

#include <opencode/types.hpp>
#include <string>
#include <variant>

namespace opencode
{

// =============================================================================
// Server Events
// =============================================================================

struct ServerConnectedEvent
{
    static constexpr const char* type = "server.connected";
};

struct ServerHeartbeatEvent
{
    static constexpr const char* type = "server.heartbeat";
};

struct ServerInstanceDisposedEvent
{
    static constexpr const char* type = "server.instance.disposed";
    std::string directory;
};

struct GlobalDisposedEvent
{
    static constexpr const char* type = "global.disposed";
};

// =============================================================================
// Session Events
// =============================================================================

struct SessionCreatedEvent
{
    static constexpr const char* type = "session.created";
    SessionInfo session;
};

struct SessionUpdatedEvent
{
    static constexpr const char* type = "session.updated";
    SessionInfo session;
};

struct SessionDeletedEvent
{
    static constexpr const char* type = "session.deleted";
    std::string session_id;
};

struct SessionStatusEvent
{
    static constexpr const char* type = "session.status";
    std::string session_id;
    SessionStatus status;
};

struct SessionIdleEvent
{
    static constexpr const char* type = "session.idle";
    std::string session_id;
};

struct SessionErrorEvent
{
    static constexpr const char* type = "session.error";
    std::string session_id;
    std::string error;
};

// =============================================================================
// Message Events
// =============================================================================

struct MessageUpdatedEvent
{
    static constexpr const char* type = "message.updated";
    Message info;
};

struct MessageRemovedEvent
{
    static constexpr const char* type = "message.removed";
    std::string session_id;
    std::string message_id;
};

struct MessagePartUpdatedEvent
{
    static constexpr const char* type = "message.part.updated";
    std::string session_id;
    std::string message_id;
    Part part;
};

struct MessagePartRemovedEvent
{
    static constexpr const char* type = "message.part.removed";
    std::string session_id;
    std::string message_id;
    std::string part_id;
};

// =============================================================================
// Permission Events
// =============================================================================

struct PermissionAskedEvent
{
    static constexpr const char* type = "permission.asked";
    PermissionRequest request;
};

struct PermissionRepliedEvent
{
    static constexpr const char* type = "permission.replied";
    std::string request_id;
    std::string session_id;
    std::string reply; // "once", "always", "reject"
};

// =============================================================================
// Project Events
// =============================================================================

struct ProjectUpdatedEvent
{
    static constexpr const char* type = "project.updated";
    Project project;
};

// =============================================================================
// File Events
// =============================================================================

struct FileEditedEvent
{
    static constexpr const char* type = "file.edited";
    std::string file;
};

// =============================================================================
// Installation Events
// =============================================================================

struct InstallationUpdatedEvent
{
    static constexpr const char* type = "installation.updated";
    std::string version;
};

struct InstallationUpdateAvailableEvent
{
    static constexpr const char* type = "installation.update-available";
    std::string version;
};

// =============================================================================
// Combined Event Type
// =============================================================================

using Event = std::variant<
    // Server
    ServerConnectedEvent,
    ServerHeartbeatEvent,
    ServerInstanceDisposedEvent,
    GlobalDisposedEvent,
    // Session
    SessionCreatedEvent,
    SessionUpdatedEvent,
    SessionDeletedEvent,
    SessionStatusEvent,
    SessionIdleEvent,
    SessionErrorEvent,
    // Message
    MessageUpdatedEvent,
    MessageRemovedEvent,
    MessagePartUpdatedEvent,
    MessagePartRemovedEvent,
    // Permission
    PermissionAskedEvent,
    PermissionRepliedEvent,
    // Project
    ProjectUpdatedEvent,
    // File
    FileEditedEvent,
    // Installation
    InstallationUpdatedEvent,
    InstallationUpdateAvailableEvent
>;

// =============================================================================
// Event Helpers
// =============================================================================

/// Get the event type string from an event
std::string get_event_type(const Event& event);

/// Check if event is a specific type (legacy)
template <typename T>
bool is_event_type(const Event& event)
{
    return std::holds_alternative<T>(event);
}

/// Check if event is a specific type
/// Usage: if (is<SessionCreatedEvent>(event)) { ... }
template <typename T>
bool is(const Event& event)
{
    return std::holds_alternative<T>(event);
}

/// Get event as specific type, returns nullptr if wrong type
/// Usage: if (auto* e = try_as<SessionCreatedEvent>(event)) { ... }
template <typename T>
const T* try_as(const Event& event)
{
    return std::get_if<T>(&event);
}

/// Get event as specific type, throws if wrong type
/// Usage: auto& e = as<SessionCreatedEvent>(event);
template <typename T>
const T& as(const Event& event)
{
    return std::get<T>(event);
}

} // namespace opencode
