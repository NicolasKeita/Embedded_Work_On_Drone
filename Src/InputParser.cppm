/*
Filename: Src/InputParser.cppm
Description: Public interface for parsing command-line arguments and retrieving option values.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

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