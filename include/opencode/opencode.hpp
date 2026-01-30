// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#pragma once

/// @file opencode.hpp
/// @brief Single header for the OpenCode C++ Client SDK
///
/// Include this header to get access to all OpenCode client functionality.
///
/// Example:
/// @code
/// #include <opencode/opencode.hpp>
/// #include <iostream>
///
/// int main()
/// {
///     opencode::Client client;  // Auto-discovers or spawns server
///
///     auto session = client.create_session();
///     auto response = session.send("What's 2 + 2?");
///     std::cout << response.text() << "\n";
/// }
/// @endcode

#include <opencode/client.hpp>
#include <opencode/events.hpp>
#include <opencode/server.hpp>
#include <opencode/session.hpp>
#include <opencode/types.hpp>
