/*
Filename: Src/App/Application.cpp
Description: Application runtime that prints help text, option values and positional arguments.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module App;

import std;
import InputParser;

Application::Application(int argc, char* argv[])
    : m_argc(argc), m_argv(argv)
{
}

int Application::Run() const
{
    InputParser parser(m_argc, m_argv);
    parser.ParseArguments();

    std::cout << "Options detectees :" << std::endl;
    if (parser.HasOption("--help"))
    {
        std::cout << "  --help : affiche l'aide" << std::endl;
    }
    if (parser.HasOption("--version"))
    {
        std::cout << "  --version : affiche la version" << std::endl;
    }

    const std::string output = parser.GetOptionValue("--output");
    if (!output.empty())
    {
        std::cout << "  --output : " << output << std::endl;
    }

    const auto positional = parser.GetPositionalArguments();
    if (!positional.empty())
    {
        std::cout << "Arguments positionnels :" << std::endl;
        for (const auto& argument : positional)
        {
            std::cout << "  " << argument << std::endl;
        }
    }

    return 0;
}
