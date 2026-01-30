// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

// Config manager example - view and update configuration

#include <opencode/opencode.hpp>
#include <iostream>
#include <iomanip>

int main()
{
    try
    {
        opencode::Client client;
        std::cout << "Configuration Manager\n";
        std::cout << "=====================\n\n";

        // Get current config
        auto config = client.get_config();

        std::cout << "Current Configuration:\n";
        std::cout << "  Default provider: " << config.default_provider.value_or("(not set)") << "\n";
        std::cout << "  Default model: " << config.default_model.value_or("(not set)") << "\n";
        std::cout << "  Auto-approve: " << (config.auto_approve.value_or(false) ? "yes" : "no") << "\n";
        if (config.max_tokens)
            std::cout << "  Max tokens: " << *config.max_tokens << "\n";
        if (config.temperature)
            std::cout << "  Temperature: " << *config.temperature << "\n";
        std::cout << "\n";

        // List configured providers
        std::cout << "Configured Providers:\n";
        auto providers = client.list_config_providers();
        for (const auto& p : providers)
        {
            std::cout << "  " << std::left << std::setw(15) << p.id;
            std::cout << (p.enabled ? "[enabled]" : "[disabled]");
            std::cout << (p.has_key ? " [key set]" : " [no key]");
            if (p.api_key_env)
                std::cout << " env: " << *p.api_key_env;
            std::cout << "\n";
        }
        std::cout << "\n";

        // List available providers with models
        std::cout << "Available Providers & Models:\n";
        auto available = client.list_providers();
        for (const auto& p : available)
        {
            std::cout << "  " << p.name << " (" << p.id << ")";
            std::cout << (p.configured ? " [configured]" : " [not configured]");
            if (p.error)
                std::cout << " ERROR: " << *p.error;
            std::cout << "\n";

            for (const auto& m : p.models)
            {
                std::cout << "    - " << m.name << " (" << m.id << ")";
                if (m.context_length)
                    std::cout << " ctx:" << *m.context_length;
                if (m.input_cost && m.output_cost)
                    std::cout << " $" << *m.input_cost << "/$" << *m.output_cost << " per 1M";
                std::cout << "\n";
            }
        }
        std::cout << "\n";

        // List modes
        std::cout << "Available Modes:\n";
        auto modes = client.list_modes();
        for (const auto& m : modes)
        {
            std::cout << "  " << m.id << ": " << m.name;
            if (m.description)
                std::cout << " - " << *m.description;
            std::cout << "\n";
        }
        std::cout << "\n";

        // List agents
        std::cout << "Available Agents:\n";
        auto agents = client.list_agents();
        for (const auto& a : agents)
        {
            std::cout << "  " << a.id << ": " << a.name;
            if (a.description)
                std::cout << " - " << *a.description;
            std::cout << "\n";
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
