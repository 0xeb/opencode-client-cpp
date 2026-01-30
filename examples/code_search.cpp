// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Code search example - search for text, files, and symbols

#include <opencode/opencode.hpp>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    try
    {
        opencode::Client client;
        std::cout << "Code Search Example\n";
        std::cout << "===================\n\n";

        std::string query = argc > 1 ? argv[1] : "TODO";

        // Search for text in files
        std::cout << "Searching for: \"" << query << "\"\n\n";

        std::cout << "=== Text Search (in *.cpp files) ===\n";
        auto text_results = client.find_text({
            .pattern = query,
            .glob = "*.cpp",
            .limit = 10,
            .case_sensitive = false
        });

        std::cout << "Found " << text_results.total_matches << " matches";
        if (text_results.truncated)
            std::cout << " (truncated)";
        std::cout << "\n\n";

        for (const auto& match : text_results.matches)
        {
            std::cout << "  " << match.path << ":" << match.line << "\n";
            std::cout << "    " << match.text << "\n";
        }

        // Search for files by pattern
        std::cout << "\n=== File Search (*.hpp) ===\n";
        auto file_results = client.find_files({
            .pattern = "**/*.hpp",
            .limit = 10
        });

        std::cout << "Found " << file_results.size() << " files:\n";
        for (const auto& f : file_results)
        {
            std::cout << "  " << (f.is_directory ? "[D] " : "    ") << f.path << "\n";
        }

        // Search for symbols (requires LSP)
        std::cout << "\n=== Symbol Search ===\n";
        try
        {
            auto symbols = client.find_symbols({
                .query = query,
                .limit = 10
            });

            std::cout << "Found " << symbols.size() << " symbols:\n";
            for (const auto& s : symbols)
            {
                std::cout << "  [" << s.kind << "] " << s.name;
                if (s.container)
                    std::cout << " (in " << *s.container << ")";
                std::cout << "\n    at " << s.path << ":" << s.line << "\n";
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "  LSP not available: " << e.what() << "\n";
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
