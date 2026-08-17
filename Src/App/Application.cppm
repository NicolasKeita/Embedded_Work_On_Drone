/*
Filename: Src/App/Application.cppm
Description: Public application interface for argument processing and runtime behavior.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module App;

import std;

export class Application
{
public:
    Application(int argc, char* argv[]);

    int Run() const;

private:
    int m_argc;
    char** m_argv;
};
