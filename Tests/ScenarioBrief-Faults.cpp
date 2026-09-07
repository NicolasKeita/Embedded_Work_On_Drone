/*
Filename: Tests/ScenarioBrief-Faults.cpp
Description: Non-technical briefs for the fault-injection scenarios
(FAULT_INJECTOR-001..005).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module ScenarioBrief;

import std;

namespace sim::test {

namespace {

struct BriefEntry {
    std::string_view id;
    std::string_view brief;
};

/*
Each brief follows the same labelled layout: Mission, Start, Goal, Faults and
Outcome. Numeric values mirror the actual scenario configuration.
*/
constexpr std::array<BriefEntry, 5> kBriefs{{
    {"FAULT_INJECTOR-001",
     "  Mission   : The drone climbs to 10 m and holds position; then, at\n"
     "              20 seconds into the flight, its main computer (FC1) dies.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Climb to and hold 10 m, then survive a sudden FC1 failure.\n"
     "  Faults    : FC1 (primary flight controller) stops working at t = 20 s\n"
     "              for the rest of the mission.\n"
     "  Outcome   : The backup watchdog must notice the failure within 0.3 s,\n"
     "              switch to safe mode and abort the mission (controlled\n"
     "              descent)."},
    {"FAULT_INJECTOR-002",
     "  Mission   : The drone climbs to 10 m and holds position; then, at\n"
     "              20 seconds, the radio link between its two computers is\n"
     "              cut.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Climb to and hold 10 m, then survive a total comms blackout.\n"
     "  Faults    : Communication between FC1 and FC2 is lost at t = 20 s.\n"
     "  Outcome   : The loss must be detected within 0.3 s, safe mode must\n"
     "              engage and the mission must be aborted."},
    {"FAULT_INJECTOR-003",
     "  Mission   : The drone climbs to 10 m and holds position; then, at\n"
     "              20 seconds, its altitude sensor starts reporting a wild,\n"
     "              impossible value for 10 seconds.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Climb to and hold 10 m, then keep flying safely despite a\n"
     "              broken altitude reading.\n"
     "  Faults    : Altitude sensor reports 99 999 m (out of range) from\n"
     "              t = 20 s to t = 30 s.\n"
     "  Outcome   : The bad reading must be rejected within 0.5 s; the drone\n"
     "              drops to a degraded but still-flying state with thrust\n"
     "              compensation. The mission is NOT aborted."},
    {"FAULT_INJECTOR-004",
     "  Mission   : The drone climbs to 10 m and holds position; then, at\n"
     "              15 seconds, its main rotor loses 40 % of its power.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Climb to and hold 10 m, then keep flying with a weakened\n"
     "              motor.\n"
     "  Faults    : Main-rotor efficiency drops to 60 % at t = 15 s\n"
     "              (permanent).\n"
     "  Outcome   : The power loss must be detected via a command/response\n"
     "              mismatch; the drone enters a degraded state with extra\n"
     "              thrust compensation. The mission is NOT aborted."},
    {"FAULT_INJECTOR-005",
     "  Mission   : The drone is climbing towards 10 m when, only 2 seconds\n"
     "              after take-off, its main computer (FC1) dies mid-climb.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Climb to 10 m, but survive an FC1 failure during the most\n"
     "              critical phase (the climb transition).\n"
     "  Faults    : FC1 (primary flight controller) stops working at t = 2 s\n"
     "              during the climb.\n"
     "  Outcome   : The failure must be detected within 0.3 s, safe mode must\n"
     "              engage and the mission must be aborted."},
}};

}

std::string_view fault_scenario_brief(std::string_view id) noexcept
{
    for (const BriefEntry& entry : kBriefs) {
        if (entry.id == id) {
            return entry.brief;
        }
    }
    return {};
}

}
