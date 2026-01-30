// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Streaming chat example - receive responses as they generate
// Set OPENCODE_URL environment variable to specify server URL

#include <opencode/opencode.hpp>
#include <iostream>
#include <string>
#include <atomic>
#include <cstdlib>

int main()
{
    try
    {
        opencode::ClientOptions opts;
        if (auto url = std::getenv("OPENCODE_URL"))
            opts.base_url = url;

        opencode::Client client(opts);

        std::cout << "Streaming Chat Example\n";
        std::cout << "======================\n\n";

        auto session = client.create_session("Streaming Test");
        std::cout << "Session: " << session.id() << "\n\n";

        std::string prompt = "Count from 1 to 5, saying each number on a new line.";
        std::cout << "You: " << prompt << "\n";
        std::cout << "AI: " << std::flush;

        std::atomic<bool> completed{false};
        std::atomic<int> part_count{0};

        bool debug = false;  // Set to true to see all parts

        // Use streaming to see tokens as they arrive
        session.send_streaming(prompt, {
            .on_part = [&part_count, debug](const opencode::Part& part)
            {
                part_count++;
                // Print text deltas as they arrive
                if (auto* text = std::get_if<opencode::TextPart>(&part))
                {
                    if (debug)
                    {
                        std::cerr << "[text part id=" << text->id
                                  << " delta=" << text->is_delta
                                  << " len=" << text->text.length() << "]\n";
                    }
                    // Only print actual deltas (streaming tokens), not full text updates
                    if (text->is_delta && !text->text.empty())
                    {
                        std::cout << text->text << std::flush;
                    }
                }
                else if (debug)
                {
                    // Log other part types
                    std::visit([](auto&& p)
                    {
                        using T = std::decay_t<decltype(p)>;
                        if constexpr (std::is_same_v<T, opencode::ToolPart>)
                            std::cerr << "[tool: " << p.tool << "]\n";
                        else if constexpr (std::is_same_v<T, opencode::ReasoningPart>)
                            std::cerr << "[reasoning]\n";
                        else
                            std::cerr << "[other part]\n";
                    }, part);
                }
            },
            .on_complete = [&completed, &part_count](const opencode::MessageWithParts& msg)
            {
                completed = true;
                std::cout << "\n\n[Complete - parts received: " << part_count.load();
                if (auto tokens = msg.tokens())
                {
                    std::cout << ", tokens: in=" << tokens->input
                              << " out=" << tokens->output;
                }
                if (auto cost = msg.cost())
                {
                    std::cout << ", cost=$" << *cost;
                }
                std::cout << "]\n";

                // Show final text for comparison
                std::cout << "\nFinal text: " << msg.text() << "\n";
            },
            .on_error = [&completed](const std::string& error)
            {
                // Ignore "Failed to read connection" after completion (expected when SSE stops)
                if (!completed && error.find("Failed to read connection") == std::string::npos)
                {
                    std::cerr << "\n[Error: " << error << "]\n";
                }
            }
        });

        session.destroy();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
