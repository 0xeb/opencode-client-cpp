// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// List sessions example - demonstrates connecting and listing sessions

#include <opencode/opencode.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        opencode::ClientOptions opts;
        // Allow specifying server URL via command line
        if (argc > 1)
        {
            opts.base_url = argv[1];
        }

        opencode::Client client(opts);
        std::cout << "Connected to " << client.server_url() << "\n\n";

        auto sessions = client.list_sessions();
        std::cout << "Sessions: " << sessions.size() << "\n";
        for (const auto& s : sessions)
        {
            std::cout << "- " << s.id << " | " << s.title << " | " << s.directory << "\n";
        }
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
