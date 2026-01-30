// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Spawn server example - demonstrates manual server lifecycle
// NOTE: With the new dedicated server model, Client() already spawns its own server.
// This example shows how to manually control server lifecycle for advanced use cases.

#include <opencode/opencode.hpp>
#include <iostream>

int main()
{
    try
    {
        std::cout << "Spawning OpenCode server manually...\n";

        // Manually spawn server (OS assigns port)
        opencode::ServerOptions server_opts;
        server_opts.port = 0;  // OS assigns port

        auto server = opencode::Server::spawn(server_opts);
        std::cout << "Server at " << server.url() << " (PID: " << server.pid() << ")\n\n";

        // Connect client to our manually-spawned server (no auto-spawn)
        opencode::Client client({.base_url = server.url()});

        auto session = client.create_session("Test");
        std::cout << "Session: " << session.id() << "\n";

        auto response = session.send("Say hello in 3 words");
        std::cout << "Response: " << response.text() << "\n";

        // Server stops automatically when 'server' goes out of scope
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
