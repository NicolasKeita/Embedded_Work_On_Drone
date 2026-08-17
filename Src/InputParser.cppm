/*
Filename: Src/InputParser.cppm
Description: Command-line argument parsing for option detection, value retrieval and positional arguments.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <string>
#include <string_view>
#include <vector>

export module InputParser;

import std;

export class InputParser
{
public:
    explicit InputParser(int argc, char* argv[]);

    void ParseArguments();

    bool HasOption(std::string_view option) const;

    std::string GetOptionValue(std::string_view option) const;

    std::vector<std::string> GetPositionalArguments() const;

private:
    std::vector<std::string> m_arguments;
    std::vector<std::string> m_positionalArguments;
};

InputParser::InputParser(int argc, char* argv[])
    : m_arguments(argv + 1, argv + argc)
{
}

void InputParser::ParseArguments()
{
    for (std::size_t index = 0; index < m_arguments.size(); ++index)
    {
        const auto& argument = m_arguments[index];

        if (argument.empty() || argument[0] != '-')
        {
            m_positionalArguments.push_back(argument);
            continue;
        }

        // Si l'option attend une valeur, on saute la valeur suivante
        if (index + 1 < m_arguments.size() && m_arguments[index + 1][0] != '-')
        {
            ++index;
        }
    }
}

bool InputParser::HasOption(std::string_view option) const
{
    return std::find(m_arguments.begin(), m_arguments.end(), option) != m_arguments.end();
}

std::string InputParser::GetOptionValue(std::string_view option) const
{
    auto iterator = std::find(m_arguments.begin(), m_arguments.end(), option);
    if (iterator == m_arguments.end())
    {
        return {};
    }

    ++iterator;
    if (iterator == m_arguments.end())
    {
        return {};
    }

    return *iterator;
}

std::vector<std::string> InputParser::GetPositionalArguments() const
{
    return m_positionalArguments;
}