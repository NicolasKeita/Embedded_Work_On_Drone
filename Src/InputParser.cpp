/*
Filename: Src/InputParser.cpp
Description: Command-line parsing implementation for option lookup and positional argument collection.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module InputParser;

import std;

InputParser::InputParser(int argc, char* argv[])
    : m_arguments(argv + 1, argv + argc)
{
}

void InputParser::ParseArguments()
{
    m_positionalArguments.clear();

    for (std::size_t index = 0; index < m_arguments.size(); ++index)
    {
        const std::string& argument = m_arguments[index];

        if (argument.empty() || argument[0] != '-')
        {
            m_positionalArguments.push_back(argument);
            continue;
        }

        if (index + 1 < m_arguments.size() && !m_arguments[index + 1].empty() && m_arguments[index + 1][0] != '-')
        {
            ++index;
        }
    }
}

bool InputParser::HasOption(std::string_view option) const
{
    return std::find(m_arguments.begin(), m_arguments.end(), std::string(option)) != m_arguments.end();
}

std::string InputParser::GetOptionValue(std::string_view option) const
{
    const auto iterator = std::find(m_arguments.begin(), m_arguments.end(), std::string(option));
    if (iterator == m_arguments.end())
    {
        return {};
    }

    const auto valueIterator = std::next(iterator);
    if (valueIterator == m_arguments.end() || valueIterator->empty() || (*valueIterator)[0] == '-')
    {
        return {};
    }

    return *valueIterator;
}

std::vector<std::string> InputParser::GetPositionalArguments() const
{
    return m_positionalArguments;
}
