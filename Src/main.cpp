/*
Filename: Src/main.cpp
Description: Program entry point for the command-line demo application.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;
import App;

int main(int argc, char* argv[])
{
    int x = 5;
    Application app(argc, argv);
    return app.Run();
}