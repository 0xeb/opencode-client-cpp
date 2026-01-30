// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Model comparison example - compare responses from different models

#include <opencode/opencode.hpp>
#include <iostream>
#include <vector>
#include <string>

struct ModelConfig
{
    std::string provider;
    std::string model;
    std::string name;
};

int main()
{
    try
    {
        opencode::Client client;

        std::cout << "Model Comparison\n";
        std::cout << "================\n\n";

        // Models to compare (adjust based on your configured providers)
        std::vector<ModelConfig> models = {
            {"zai-coding-plan", "glm-4.7", "GLM-4.7"},
            {"zai-coding-plan", "glm-4.5-flash", "GLM-4.5 Flash"},
        };

        std::string prompt = "What is the capital of France? Answer in one word.";
        std::cout << "Prompt: " << prompt << "\n\n";

        for (const auto& m : models)
        {
            try
            {
                auto session = client.create_session("Model Test: " + m.name);
                auto response = session.send(prompt, m.provider, m.model);

                std::cout << m.name << ":\n";
                std::cout << "  Response: " << response.text() << "\n";

                if (auto tokens = response.tokens())
                {
                    std::cout << "  Tokens: in=" << tokens->input
                              << " out=" << tokens->output << "\n";
                }
                std::cout << "\n";

                session.destroy();
            }
            catch (const std::exception& e)
            {
                std::cout << m.name << ": Error - " << e.what() << "\n\n";
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
