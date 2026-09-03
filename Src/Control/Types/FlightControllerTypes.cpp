/*
Filename: Src/Control/Types/FlightControllerTypes.cpp
Description: Mission state naming helper of the flight controller data types.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightControllerTypes;

import std;

namespace sim::control {

/*
Mission state names. ABORTED and FAILED are SIL-level terminal states produced
by the simulation engine (safety abort, mission window exhausted); the onboard
controller itself never enters them.
*/
std::string_view mission_state_name(MissionState state)
{
    switch (state) {
    case MissionState::TAKEOFF:
        return "TAKEOFF";
    case MissionState::CLIMB:
        return "CLIMB";
    case MissionState::STATION_KEEPING:
        return "STATION_KEEPING";
    case MissionState::COMPLETE:
        return "COMPLETE";
    case MissionState::ABORTED:
        return "ABORTED";
    case MissionState::FAILED:
        return "FAILED";
    }
    return "UNKNOWN";
}

}
