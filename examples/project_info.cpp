// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Project info example - display project and provider information

#include <opencode/opencode.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        opencode::ClientOptions opts;
        if (argc > 1) opts.base_url = argv[1];
        opencode::Client client(opts);

        std::cout << "Server: " << client.server_url() << "\n\n";

        // Health info
        auto health = client.health();
        std::cout << "Version: " << health.version << "\n";
        std::cout << "Healthy: " << (health.healthy ? "yes" : "no") << "\n\n";

        // Current project
        std::cout << "Current Project:\n";
        auto project = client.current_project();
        std::cout << "  ID: " << project.id << "\n";
        std::cout << "  Path: " << project.worktree << "\n";
        if (project.name)
            std::cout << "  Name: " << *project.name << "\n";
        if (project.vcs)
            std::cout << "  VCS: " << *project.vcs << "\n";
        std::cout << "\n";

        // All projects
        auto projects = client.list_projects();
        std::cout << "All Projects (" << projects.size() << "):\n";
        for (const auto& p : projects)
        {
            std::cout << "  - " << p.worktree;
            if (p.name)
                std::cout << " (" << *p.name << ")";
            std::cout << "\n";
        }
        std::cout << "\n";

        // Sessions summary
        auto sessions = client.list_sessions();
        std::cout << "Sessions: " << sessions.size() << " total\n";

        int recent = 0;
        for (const auto& s : sessions)
        {
            if (++recent > 5) break;
            std::cout << "  - " << s.title << " [" << s.id.substr(0, 12) << "...]\n";
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
