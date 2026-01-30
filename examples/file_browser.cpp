// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// File browser example - browse and read files via the API

#include <opencode/opencode.hpp>
#include <iostream>
#include <iomanip>

void print_size(int64_t bytes)
{
    if (bytes < 1024)
        std::cout << bytes << " B";
    else if (bytes < 1024 * 1024)
        std::cout << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
    else
        std::cout << std::fixed << std::setprecision(1) << (bytes / 1024.0 / 1024.0) << " MB";
}

int main(int argc, char* argv[])
{
    try
    {
        opencode::Client client;
        std::cout << "Connected to " << client.server_url() << "\n\n";

        std::string path = argc > 1 ? argv[1] : ".";

        // List files in directory
        std::cout << "Contents of: " << path << "\n";
        std::cout << std::string(50, '-') << "\n";

        auto files = client.list_files(path);
        for (const auto& f : files)
        {
            std::cout << (f.is_directory ? "[DIR] " : "      ");
            std::cout << std::left << std::setw(30) << f.name;
            if (f.size && !f.is_directory)
            {
                print_size(*f.size);
            }
            std::cout << "\n";
        }

        std::cout << "\nTotal: " << files.size() << " items\n\n";

        // Try to read a README if it exists
        for (const auto& f : files)
        {
            if (f.name == "README.md" || f.name == "readme.md")
            {
                std::cout << "Reading " << f.name << ":\n";
                std::cout << std::string(50, '-') << "\n";
                auto content = client.read_file(f.path);
                // Print first 500 chars
                std::cout << content.content.substr(0, 500);
                if (content.content.size() > 500)
                    std::cout << "\n... (" << content.content.size() << " bytes total)";
                std::cout << "\n";
                break;
            }
        }

        // Show git status for a file
        std::cout << "\nFile status:\n";
        for (const auto& f : files)
        {
            if (!f.is_directory)
            {
                try
                {
                    auto status = client.file_status(f.path);
                    if (status.status != "clean")
                    {
                        std::cout << "  " << f.name << ": " << status.status;
                        if (status.additions)
                            std::cout << " (+" << *status.additions << ")";
                        if (status.deletions)
                            std::cout << " (-" << *status.deletions << ")";
                        std::cout << "\n";
                    }
                }
                catch (...)
                {
                    // Ignore errors
                }
            }
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
