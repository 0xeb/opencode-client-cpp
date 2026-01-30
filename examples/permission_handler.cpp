// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Permission handler example - monitor and auto-approve permissions

#include <opencode/opencode.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

std::atomic<bool> g_running{true};

void permission_monitor(opencode::Client& client)
{
    while (g_running)
    {
        try
        {
            auto permissions = client.list_permissions();
            for (const auto& req : permissions)
            {
                std::cout << "[Permission] " << req.permission;
                if (!req.patterns.empty())
                    std::cout << " (" << req.patterns[0] << ")";
                std::cout << "\n";

                // Auto-approve with "always" for this session
                opencode::PermissionReply reply;
                reply.request_id = req.id;
                reply.action = opencode::PermissionAction::Always;

                if (client.reply_permission(reply))
                    std::cout << "  -> Approved\n";
            }
        }
        catch (...)
        {
            // Ignore errors during polling
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main()
{
    try
    {
        opencode::Client client;

        std::cout << "Connected to " << client.server_url() << "\n";
        std::cout << "Starting permission monitor...\n\n";

        // Start permission monitor in background
        std::thread monitor_thread(permission_monitor, std::ref(client));

        // Create session and send a message that might trigger permissions
        auto session = client.create_session("Permission Test");

        std::cout << "Sending message that may trigger tool use...\n";
        auto response = session.send("List the files in the current directory");
        std::cout << "\nResponse: " << response.text() << "\n";

        // Cleanup
        g_running = false;
        monitor_thread.join();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
