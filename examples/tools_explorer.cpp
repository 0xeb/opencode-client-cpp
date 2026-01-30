// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Tools explorer example - list and inspect available tools

#include <opencode/opencode.hpp>
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[])
{
    try
    {
        opencode::Client client;
        std::cout << "Tools Explorer\n";
        std::cout << "==============\n\n";

        std::string filter = argc > 1 ? argv[1] : "";

        // Get full tool info
        auto tools = client.list_tools();

        std::cout << "Available Tools: " << tools.size() << "\n";
        if (!filter.empty())
            std::cout << "Filter: \"" << filter << "\"\n";
        std::cout << "\n";

        int shown = 0;
        for (const auto& tool : tools)
        {
            // Apply filter
            if (!filter.empty())
            {
                if (tool.name.find(filter) == std::string::npos &&
                    tool.id.find(filter) == std::string::npos)
                    continue;
            }

            std::cout << "Tool: " << tool.name;
            if (tool.name != tool.id)
                std::cout << " (" << tool.id << ")";
            std::cout << "\n";

            if (tool.category)
                std::cout << "  Category: " << *tool.category << "\n";
            if (tool.description)
                std::cout << "  Description: " << *tool.description << "\n";

            std::cout << "  Enabled: " << (tool.enabled ? "yes" : "no") << "\n";

            if (!tool.parameters.empty())
            {
                std::cout << "  Parameters:\n";
                for (const auto& param : tool.parameters)
                {
                    std::cout << "    " << param.name << " (" << param.type << ")";
                    if (param.required)
                        std::cout << " [required]";
                    if (param.default_value)
                        std::cout << " default=" << *param.default_value;
                    std::cout << "\n";
                    if (param.description)
                        std::cout << "      " << *param.description << "\n";
                }
            }
            std::cout << "\n";
            shown++;
        }

        if (shown == 0 && !filter.empty())
        {
            std::cout << "No tools matching \"" << filter << "\"\n";
        }

        // Also show LSP status
        std::cout << "=== LSP Status ===\n";
        try
        {
            auto lsp = client.lsp_status();
            if (lsp.servers.empty())
            {
                std::cout << "No LSP servers running.\n";
            }
            else
            {
                for (const auto& s : lsp.servers)
                {
                    std::cout << "  " << s.language << ": " << s.name << " [" << s.status << "]";
                    if (s.version)
                        std::cout << " v" << *s.version;
                    if (s.pid)
                        std::cout << " (pid " << *s.pid << ")";
                    std::cout << "\n";
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "  Error: " << e.what() << "\n";
        }

        // And formatter status
        std::cout << "\n=== Formatter Status ===\n";
        try
        {
            auto fmt = client.formatter_status();
            if (fmt.formatters.empty())
            {
                std::cout << "No formatters available.\n";
            }
            else
            {
                for (const auto& f : fmt.formatters)
                {
                    std::cout << "  " << f.language << ": " << f.name << " [" << f.status << "]";
                    if (f.version)
                        std::cout << " v" << *f.version;
                    std::cout << "\n";
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "  Error: " << e.what() << "\n";
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
