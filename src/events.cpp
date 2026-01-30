// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#include <opencode/events.hpp>

namespace opencode
{

std::string get_event_type(const Event& event)
{
    return std::visit(
        [](const auto& e) -> std::string
        {
            using T = std::decay_t<decltype(e)>;
            return T::type;
        },
        event);
}

} // namespace opencode
