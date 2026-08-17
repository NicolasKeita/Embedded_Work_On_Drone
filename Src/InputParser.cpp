#include "InputParser.h"

#include <algorithm>

InputParser::InputParser(int argc, char* argv[])
    : m_arguments(argv + 1, argv + argc)
{
}

void InputParser::ParseArguments()
{
    for (const auto& argument : m_arguments)
    {
        if (argument.empty() || argument[0] != '-')
        {
            m_positionalArguments.push_back(argument);
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