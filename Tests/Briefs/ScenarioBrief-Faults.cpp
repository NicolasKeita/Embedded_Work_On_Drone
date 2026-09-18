/*
Filename: Tests/Briefs/ScenarioBrief-Faults.cpp
Description: Non-technical briefs for the two shared fault-injection scenarios.

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
constexpr std::array<BriefEntry, 2> kBriefs{{
    {"FAULT_INJECTOR-001",
     "  Mission   : The drone climbs to 10 m and holds position; then, at\n"
     "              40 seconds into the flight, its main computer (FC1) dies.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Climb to and hold 10 m, then survive a sudden FC1 failure.\n"
     "  Faults    : FC1 (primary flight controller) stops working at t = 40 s\n"
     "              for the rest of the mission.\n"
     "  Outcome   : The backup heartbeat supervision must notice the failure within 0.3 s,\n"
     "              switch to safe mode and abort the mission (the drone holds\n"
     "              its position)."},
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
     "              compensation. The mission is NOT aborted."}, }};

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
