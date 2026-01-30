// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// MCP manager example - manage Model Context Protocol servers

#include <opencode/opencode.hpp>
#include <iostream>

int main()
{
    try
    {
        opencode::Client client;
        std::cout << "MCP Server Manager\n";
        std::cout << "==================\n\n";

        // Get MCP status
        auto status = client.mcp_status();

        if (status.servers.empty())
        {
            std::cout << "No MCP servers configured.\n\n";
            std::cout << "To add an MCP server, use:\n";
            std::cout << "  client.mcp_add({\n";
            std::cout << "      .name = \"my-server\",\n";
            std::cout << "      .command = \"npx\",\n";
            std::cout << "      .args = {\"-y\", \"@modelcontextprotocol/server-filesystem\"}\n";
            std::cout << "  });\n\n";
        }
        else
        {
            std::cout << "MCP Servers: " << status.servers.size() << "\n\n";

            for (const auto& server : status.servers)
            {
                std::cout << "Server: " << server.name << " (" << server.id << ")\n";
                std::cout << "  Status: " << server.status << "\n";
                if (server.error)
                    std::cout << "  Error: " << *server.error << "\n";

                if (!server.tools.empty())
                {
                    std::cout << "  Tools (" << server.tools.size() << "):\n";
                    for (const auto& tool : server.tools)
                    {
                        std::cout << "    - " << tool.name;
                        if (tool.description)
                            std::cout << ": " << tool.description->substr(0, 50);
                        std::cout << "\n";
                    }
                }

                if (!server.resources.empty())
                {
                    std::cout << "  Resources (" << server.resources.size() << "):\n";
                    for (const auto& res : server.resources)
                    {
                        std::cout << "    - " << res.name << " (" << res.uri << ")";
                        if (res.mime_type)
                            std::cout << " [" << *res.mime_type << "]";
                        std::cout << "\n";
                    }
                }
                std::cout << "\n";
            }
        }

        // Show available tools from all sources
        std::cout << "All Available Tools:\n";
        auto tool_ids = client.list_tool_ids();
        std::cout << "  Total tools: " << tool_ids.size() << "\n";
        for (size_t i = 0; i < std::min(tool_ids.size(), size_t(10)); ++i)
        {
            std::cout << "    - " << tool_ids[i] << "\n";
        }
        if (tool_ids.size() > 10)
            std::cout << "    ... and " << (tool_ids.size() - 10) << " more\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
