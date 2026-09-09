/*
Filename: Tests/Briefs/ScenarioBrief-Autonomous.cpp
Description: Non-technical briefs for the autonomous flight scenarios
(NOMINAL-008..011).

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
constexpr std::array<BriefEntry, 4> kBriefs{{
    {"NOMINAL-008",
     "  Mission   : The drone flies on its own to a point 20 m forward at\n"
     "              100 m altitude, then returns horizontally to x = 0.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Reach (20, 0, 100), then bring X back to 0 (+/- 0.5 m)\n"
     "              while holding 100 m altitude.\n"
     "  Faults    : None.\n"
     "  Outcome   : The autonomous controller converges to x = 0 with less than\n"
     "              15 % overshoot and under 0.5 m residual error."},
    {"NOMINAL-009",
     "  Mission   : The drone flies on its own to a point 15 m to the side at\n"
     "              100 m altitude, then returns horizontally to y = 0.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Reach (0, -15, 100), then bring Y back to 0 (+/- 0.5 m)\n"
     "              while holding 100 m altitude.\n"
     "  Faults    : None.\n"
     "  Outcome   : The autonomous controller converges to y = 0 with less than\n"
     "              15 % overshoot and under 0.5 m residual error."},
    {"NOMINAL-010",
     "  Mission   : The drone flies a full autonomous mission from a starting\n"
     "              point to a target, going through take-off, climb and\n"
     "              station-keeping, ending completed.\n"
     "  Start     : On the ground at (20, -15, 0), motors off.\n"
     "  Goal      : Fly to and hold position (0, 0, 100) within +/- 1 m on\n"
     "              every axis.\n"
     "  Faults    : None.\n"
     "  Outcome   : The mission passes through TAKEOFF -> CLIMB ->\n"
     "              STATION_KEEPING -> COMPLETE and the drone ends stable at\n"
     "              (0, 0, 100)."},
    {"NOMINAL-011",
     "  Mission   : The drone climbs on its own from the ground to 100 m and\n"
     "              then holds that altitude steady.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Reach and hold 100 m altitude (+/- 2 m) within a 180-second\n"
     "              window.\n"
     "  Faults    : None.\n"
     "  Outcome   : The drone reaches 100 m with less than 10 % overshoot and\n"
     "              under 1 m residual error, then holds station."}, }};

}

std::string_view autonomous_scenario_brief(std::string_view id) noexcept
{
    for (const BriefEntry& entry : kBriefs) {
        if (entry.id == id) {
            return entry.brief;
        }
    }
    return {};
}

}
