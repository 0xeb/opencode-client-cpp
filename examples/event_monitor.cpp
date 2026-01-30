// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Event monitor example - subscribe to server events via SSE

#include <opencode/opencode.hpp>

#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running{true};

void signal_handler(int)
{
    g_running = false;
}

int main()
{
    // Handle Ctrl+C gracefully
    std::signal(SIGINT, signal_handler);

    try
    {
        opencode::Client client;
        std::cout << "Connected to " << client.server_url() << "\n";
        std::cout << "Press Ctrl+C to exit.\n\n";

        // Subscribe to events
        auto events = client.subscribe_events();

        std::cout << "Listening for events...\n\n";

        for (const auto& event : events)
        {
            if (!g_running)
                break;

            // Use try_as<T> for clean type-safe access (inspired by copilot-sdk-cpp)
            if (opencode::is<opencode::ServerConnectedEvent>(event))
            {
                std::cout << "[server.connected]\n";
            }
            else if (opencode::is<opencode::ServerHeartbeatEvent>(event))
            {
                std::cout << "[heartbeat]\n";
            }
            else if (auto* e = opencode::try_as<opencode::SessionCreatedEvent>(event))
            {
                std::cout << "[session.created] " << e->session.id
                          << " - " << e->session.title << "\n";
            }
            else if (auto* e = opencode::try_as<opencode::SessionUpdatedEvent>(event))
            {
                std::cout << "[session.updated] " << e->session.id << "\n";
            }
            else if (auto* e = opencode::try_as<opencode::MessagePartUpdatedEvent>(event))
            {
                std::cout << "[message.part.updated] session=" << e->session_id
                          << " msg=" << e->message_id << "\n";
            }
            else if (auto* e = opencode::try_as<opencode::PermissionAskedEvent>(event))
            {
                std::cout << "[permission.asked] " << e->request.permission
                          << " for session " << e->request.session_id << "\n";

                // Auto-approve
                opencode::PermissionReply reply;
                reply.request_id = e->request.id;
                reply.action = opencode::PermissionAction::Always;

                if (client.reply_permission(reply))
                {
                    std::cout << "  -> Auto-approved!\n";
                }
            }
            else if (auto* e = opencode::try_as<opencode::PermissionRepliedEvent>(event))
            {
                std::cout << "[permission.replied] " << e->request_id
                          << " -> " << e->reply << "\n";
            }
            else if (auto* e = opencode::try_as<opencode::ProjectUpdatedEvent>(event))
            {
                std::cout << "[project.updated] " << e->project.id << "\n";
            }
            else if (auto* e = opencode::try_as<opencode::FileEditedEvent>(event))
            {
                std::cout << "[file.edited] " << e->file << "\n";
            }
            else
            {
                std::cout << "[event] " << opencode::get_event_type(event) << "\n";
            }
        }

        std::cout << "\nDisconnected.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
