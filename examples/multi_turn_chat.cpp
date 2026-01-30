// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Multi-turn chat example - demonstrates the clean Session API
// Set OPENCODE_URL environment variable to specify server URL

#include <opencode/opencode.hpp>
#include <iostream>
#include <cstdlib>

int main()
{
    try
    {
        // Create client - uses OPENCODE_URL env var or auto-discovers
        opencode::ClientOptions opts;
        if (auto url = std::getenv("OPENCODE_URL"))
            opts.base_url = url;

        opencode::Client client(opts);

        std::cout << "Connected to " << client.server_url() << "\n\n";

        // Create a session
        auto session = client.create_session("Multi-turn Chat");
        std::cout << "Session: " << session.id() << "\n\n";

        // Multi-turn conversation - just use session.send()!
        auto r1 = session.send("What's 2 + 2?");
        std::cout << "Q: What's 2 + 2?\n";
        std::cout << "A: " << r1.text() << "\n\n";

        auto r2 = session.send("Multiply that by 10");
        std::cout << "Q: Multiply that by 10\n";
        std::cout << "A: " << r2.text() << "\n\n";

        auto r3 = session.send("What numbers did I ask about?");
        std::cout << "Q: What numbers did I ask about?\n";
        std::cout << "A: " << r3.text() << "\n\n";

        // Show token usage
        if (auto tokens = r3.tokens())
        {
            std::cout << "[Total tokens: in=" << tokens->input
                      << " out=" << tokens->output << "]\n";
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
