// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#include <opencode/types.hpp>

#include <stdexcept>

namespace opencode
{

std::string permission_action_to_string(PermissionAction action)
{
    switch (action)
    {
    case PermissionAction::Once:
        return "once";
    case PermissionAction::Always:
        return "always";
    case PermissionAction::Reject:
        return "reject";
    default:
        return "once";
    }
}

PermissionAction string_to_permission_action(const std::string& str)
{
    if (str == "once")
        return PermissionAction::Once;
    if (str == "always")
        return PermissionAction::Always;
    if (str == "reject")
        return PermissionAction::Reject;
    return PermissionAction::Once;
}

} // namespace opencode
