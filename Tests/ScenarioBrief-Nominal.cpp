/*
Filename: Tests/ScenarioBrief-Nominal.cpp
Description: Non-technical briefs for the nominal scenarios (NOMINAL-001..007).

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
constexpr std::array<BriefEntry, 7> kBriefs{{
    {"NOMINAL-001",
     "  Mission   : The drone takes off from the ground, climbs to 10 m and\n"
     "              holds its position steady at that altitude.\n"
     "  Start     : On the ground at position (0, 0, 0), motors off.\n"
     "  Goal      : Reach and hold 10 m altitude for the full 30-second mission.\n"
     "  Faults    : None - this is the normal, no-fault reference run.\n"
     "  Outcome   : The mission should complete successfully with the drone\n"
     "              stable near 10 m."},
    {"NOMINAL-002",
     "  Mission   : The drone sits on the ground with its motors switched off\n"
     "              and must not move at all.\n"
     "  Start     : On the ground at (0, 0, 0), motors off, servos centred.\n"
     "  Goal      : Stay perfectly still on the ground for 3 seconds.\n"
     "  Faults    : None.\n"
     "  Outcome   : Zero altitude, zero velocity, zero tilt - the drone does\n"
     "              not budge."},
    {"NOMINAL-003",
     "  Mission   : The drone climbs straight up under manual throttle to see\n"
     "              that more rotor speed lifts it off the ground.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Gain altitude (above 1 m) within 6 seconds of powered\n"
     "              climb.\n"
     "  Faults    : None.\n"
     "  Outcome   : The drone rises off the ground with a positive climb rate\n"
     "              and rotor speed above hover."},
    {"NOMINAL-004",
     "  Mission   : The drone takes off, climbs, then throttles down to come\n"
     "              back to the ground.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Climb, then descend and touch down again (z = 0 m).\n"
     "  Faults    : None.\n"
     "  Outcome   : After the climb the drone descends, its vertical speed\n"
     "              turns negative and it lands back on the ground."},
    {"NOMINAL-005",
     "  Mission   : The drone takes off, then tilts its nose forward to fly\n"
     "              in the forward (X) direction.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Move forward more than 1 m along the X axis without\n"
     "              drifting sideways.\n"
     "  Faults    : None.\n"
     "  Outcome   : Positive pitch, forward velocity and X displacement; no\n"
     "              sideways drift."},
    {"NOMINAL-006",
     "  Mission   : The drone takes off, then tilts sideways to fly in the\n"
     "              lateral (Y) direction.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Move sideways more than 1 m along the Y axis without\n"
     "              drifting forward.\n"
     "  Faults    : None.\n"
     "  Outcome   : Positive roll, lateral velocity and Y displacement; no\n"
     "              forward drift."},
    {"NOMINAL-007",
     "  Mission   : The drone takes off, then tilts both forward and sideways\n"
     "              at the same time while still climbing.\n"
     "  Start     : On the ground at (0, 0, 0), motors off.\n"
     "  Goal      : Combine a climb with forward (X) and lateral (Y) movement.\n"
     "  Faults    : None.\n"
     "  Outcome   : Nose-down pitch, opposite-side roll, climbing speed, and\n"
     "              displacement in both X and Y."},
}};

}

std::string_view nominal_scenario_brief(std::string_view id) noexcept
{
    for (const BriefEntry& entry : kBriefs) {
        if (entry.id == id) {
            return entry.brief;
        }
    }
    return {};
}

}
