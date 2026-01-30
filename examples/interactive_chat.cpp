// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Interactive chat with streaming, tools, and permissions

#include <opencode/opencode.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <set>

std::atomic<bool> g_running{true};
std::mutex g_cout_mutex;

void event_monitor(opencode::Client& client)
{
    try
    {
        auto events = client.subscribe_events();

        for (const auto& event : events)
        {
            if (!g_running)
                break;

            std::lock_guard<std::mutex> lock(g_cout_mutex);

            if (auto* e = opencode::try_as<opencode::PermissionAskedEvent>(event))
            {
                std::cout << "\n  [Permission: " << e->request.permission;
                if (!e->request.patterns.empty())
                    std::cout << " " << e->request.patterns[0];
                std::cout << "] ";

                client.reply_permission({
                    .request_id = e->request.id,
                    .action = opencode::PermissionAction::Always
                });
                std::cout << "approved\n" << std::flush;
            }
        }
    }
    catch (...)
    {
    }
}

std::string truncate(const std::string& s, size_t max_len)
{
    std::string result = s;
    // Remove newlines
    for (char& c : result)
        if (c == '\n' || c == '\r') c = ' ';
    if (result.size() > max_len)
        result = result.substr(0, max_len) + "...";
    return result;
}

int main()
{
    try
    {
        opencode::Client client;

        std::cout << "Connected to " << client.server_url() << "\n";
        std::cout << "Type 'quit' to exit, 'new' for new session\n\n";

        std::thread monitor(event_monitor, std::ref(client));
        monitor.detach();

        auto session = client.create_session("Interactive Chat");
        std::cout << "Session: " << session.id() << "\n\n";

        std::string line;
        std::cout << "> ";
        while (std::getline(std::cin, line))
        {
            if (line == "quit" || line == "exit")
            {
                g_running = false;
                break;
            }

            if (line == "new")
            {
                session = client.create_session("Interactive Chat");
                std::cout << "New session: " << session.id() << "\n> ";
                continue;
            }

            if (line.empty())
            {
                std::cout << "> ";
                continue;
            }

            std::cout << "\n";

            std::string last_thinking;
            std::set<std::string> shown_tools;

            session.send_streaming(line, {
                .on_part = [&](const opencode::Part& part)
                {
                    std::lock_guard<std::mutex> lock(g_cout_mutex);

                    if (auto* text = std::get_if<opencode::TextPart>(&part))
                    {
                        if (text->is_delta)
                            std::cout << text->text << std::flush;
                    }
                    else if (auto* reasoning = std::get_if<opencode::ReasoningPart>(&part))
                    {
                        last_thinking = reasoning->text;
                    }
                    else if (auto* tool = std::get_if<opencode::ToolPart>(&part))
                    {
                        std::string status = tool->state ? tool->state->status : "?";
                        std::string key = tool->id + ":" + status;
                        
                        if (shown_tools.find(key) == shown_tools.end())
                        {
                            shown_tools.insert(key);
                            
                            if (status == "running")
                            {
                                std::cout << "  [" << tool->tool << "] ";
                                // Show the command/input
                                for (const auto& [k, v] : tool->input)
                                {
                                    std::cout << truncate(v, 70);
                                    break;
                                }
                                std::cout << "\n" << std::flush;
                            }
                            else if (status == "completed")
                            {
                                std::cout << "  [" << tool->tool << "] done\n" << std::flush;
                            }
                        }
                    }
                },
                .on_complete = [&](const opencode::MessageWithParts& msg)
                {
                    std::lock_guard<std::mutex> lock(g_cout_mutex);
                    
                    if (!last_thinking.empty())
                        std::cout << "\n  (thinking: " << truncate(last_thinking, 80) << ")\n";
                    
                    std::cout << "\n> " << std::flush;
                },
                .on_error = [](const std::string& error)
                {
                    if (!error.empty() && error.find("closed") == std::string::npos)
                        std::cerr << "\n  [Error: " << error << "]\n";
                }
            });
        }

        g_running = false;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
