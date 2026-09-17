/*
Filename: apps/fc1_stm32/src/main.cpp
Description: Minimal Zephyr entry point for the FC1 firmware runtime.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import Fc1Firmware;

/* Starts the FC1 firmware runtime. */
int main()
{
    return run_fc1_firmware();
}
