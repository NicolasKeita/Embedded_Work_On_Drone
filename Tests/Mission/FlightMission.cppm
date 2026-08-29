/*
Filename: Tests/Mission/FlightMission.cppm
Description: Autonomous full-mission scenario J : TAKEOFF to COMPLETE state sequence.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FlightMission;

import TestHarness;

export namespace sim::test::flight_scenarios {

// Scenario J : mission complete TAKEOFF -> CLIMB -> STATION_KEEPING -> COMPLETE.
void autonomous_mission(TestHarness& runner, double hover_rpm);

}
