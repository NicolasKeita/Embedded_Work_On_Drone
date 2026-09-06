/*
Filename: Src/Embedded/Hil/FcHost/main.cpp
Description: Entry point of fc1_hil_host : the host FC emulator target (the stdio frame
loop itself lives in the FcHostApp module).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import FcHostApp;

int main()
{
    return sim::hil::run_fc1_host();
}
