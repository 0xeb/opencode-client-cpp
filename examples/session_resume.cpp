// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Session resume example - continue a previous conversation

#include <opencode/opencode.hpp>
#include <iostream>
#include <fstream>

const char* SESSION_FILE = "last_session.txt";

std::string load_session_id()
{
    std::ifstream f(SESSION_FILE);
    std::string id;
    if (f >> id)
        return id;
    return {};
}

void save_session_id(const std::string& id)
{
    std::ofstream f(SESSION_FILE);
    f << id;
}

int main(int argc, char* argv[])
{
    try
    {
        opencode::Client client;

        opencode::Session session = [&]() {
            // Try to resume previous session
            auto last_id = load_session_id();
            if (!last_id.empty())
            {
                try
                {
                    auto s = client.get_session(last_id);
                    std::cout << "Resumed session: " << s.id() << "\n";

                    // Show last few messages
                    auto msgs = s.messages(4);
                    if (!msgs.empty())
                    {
                        std::cout << "\nRecent history:\n";
                        for (const auto& m : msgs)
                        {
                            bool is_user = std::holds_alternative<opencode::UserMessage>(m.info);
                            std::cout << (is_user ? "You: " : "AI: ") << m.text().substr(0, 50);
                            if (m.text().size() > 50) std::cout << "...";
                            std::cout << "\n";
                        }
                    }
                    std::cout << "\n";
                    return s;
                }
                catch (...)
                {
                    std::cout << "Previous session not found, creating new one\n";
                }
            }

            auto s = client.create_session("Resumable Chat");
            std::cout << "New session: " << s.id() << "\n\n";
            return s;
        }();

        save_session_id(session.id());

        // Send a message
        std::string prompt = argc > 1 ? argv[1] : "Hello! What were we talking about?";
        std::cout << "You: " << prompt << "\n";

        auto response = session.send(prompt);
        std::cout << "AI: " << response.text() << "\n";

        std::cout << "\n(Session saved. Run again to continue.)\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
