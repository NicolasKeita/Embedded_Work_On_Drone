/*
Filename: apps/fc2_stm32/src/main.cpp
Description: Minimal Zephyr entry point for the FC2 firmware runtime.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import Fc2Firmware;

/* Starts the FC2 firmware runtime. */
int main()
{
    return run_fc2_firmware();
}
