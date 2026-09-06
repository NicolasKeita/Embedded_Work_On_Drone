/*
Filename: Src/Embedded/main.cpp
Description: hil_step_smoke_test entry point : runs ONE full lockstep step
(SensorPacket -> parser -> ISensorInput -> placeholder FC -> IActuatorOutput -> ActuatorPacket)
through the Demo module. This is a one-step protocol smoke test (packet encoding/decoding,
transport connectivity, one-step closure), NOT the mission-level HIL runner, and it uses a
placeholder control law, not the real Flight Controller core.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import Demo;

int main()
{
    return FlightCore::Demo::runLockstepStep();
}
