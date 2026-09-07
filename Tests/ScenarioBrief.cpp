/*
Filename: Tests/ScenarioBrief.cpp
Description: Dispatcher of the scenario brief lookup: delegates to the nominal,
autonomous and fault-injection brief tables defined in the split implementation files.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module ScenarioBrief;

import std;

namespace sim::test {

/*
Resolves a scenario brief by trying the nominal table first, then the
autonomous table, then the fault-injection table. Returns an empty view when
none of the tables knows the ID.
*/
std::string_view scenario_brief(std::string_view id) noexcept
{
    const std::string_view nominal = nominal_scenario_brief(id);
    if (!nominal.empty()) {
        return nominal;
    }
    const std::string_view autonomous = autonomous_scenario_brief(id);
    if (!autonomous.empty()) {
        return autonomous;
    }
    return fault_scenario_brief(id);
}

}
