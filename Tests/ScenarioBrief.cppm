/*
Filename: Tests/ScenarioBrief.cppm
Description: Human-readable, non-technical scenario briefs keyed by standardised
scenario ID. Each brief explains the mission, the starting and target state, the
altitude-hold requirement and the expected outcome so a non-technical reader can
understand what a scenario does before it runs.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module ScenarioBrief;

import std;

export namespace sim::test {

/*
Returns the human-readable brief for the given standardised scenario ID, or an
empty view when the ID has no registered brief. The brief is multi-line plain
English aimed at a non-technical reader and is printed before the scenario runs.
*/
[[nodiscard]] std::string_view scenario_brief(std::string_view id) noexcept;

}

namespace sim::test {

/*
Module-internal lookup of the nominal scenario briefs (NOMINAL-001..007).
Returns an empty view when the ID is not a nominal scenario.
*/
[[nodiscard]] std::string_view nominal_scenario_brief(std::string_view id) noexcept;

/*
Module-internal lookup of the autonomous scenario briefs (NOMINAL-008..011).
Returns an empty view when the ID is not an autonomous scenario.
*/
[[nodiscard]] std::string_view autonomous_scenario_brief(std::string_view id) noexcept;

/*
Module-internal lookup of the fault-injection scenario briefs (FAULT_INJECTOR-001..005).
Returns an empty view when the ID is not a fault-injection scenario.
*/
[[nodiscard]] std::string_view fault_scenario_brief(std::string_view id) noexcept;

}
