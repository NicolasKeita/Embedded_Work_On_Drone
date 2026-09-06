/*
Filename: Src/Embedded/Main.cpp
Description: HIL step-closure demo entry point: runs one full lockstep step
(SensorPacket -> parser -> ISensorInput -> FC factice -> IActuatorOutput ->
ActuatorPacket) through the Demo module and reports round-trip timing.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import Demo;

int main()
{
    return FlightCore::Demo::runLockstepStep();
}
