/*
Filename: Src/Safety/SafetyManager.cpp
Description: Safety mode transitions and response commands of the safety manager.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SafetyManager;

import std;

import HealthMonitor;

namespace sim::safety {

std::string_view safety_mode_name(SafetyMode mode)
{
    switch (mode) {
    case SafetyMode::NORMAL:
        return "NORMAL";
    case SafetyMode::COMPENSATED:
        return "COMPENSATED";
    case SafetyMode::SAFE_MODE:
        return "SAFE_MODE";
    }
    return "UNKNOWN";
}

SafetyManager::SafetyManager(SafetyManagerConfig config) : config_{config} {}

SafetyMode SafetyManager::mode() const noexcept
{
    return mode_;
}

double SafetyManager::response_time() const noexcept
{
    return response_time_;
}

void SafetyManager::engage(double current_time, SafetyMode mode, double margin, bool abort)
{
    if (mode != SafetyMode::NORMAL && response_time_ < 0.0) {
        response_time_ = current_time;
    }
    mode_ = mode;
    command_ = SafetyCommand{.mission_abort = abort, .thrust_margin = margin};
}

/*
Safety transitions: SAFE -> SAFE_MODE (mission abort, conservative),
DEGRADED -> COMPENSATED with a thrust margin only on proven actuator mismatch
(otherwise the fault is a sensor one and thrust must stay nominal),
HEALTHY -> NORMAL. The SAFE_MODE transition is irreversible.
*/
SafetyCommand SafetyManager::update(double current_time, const HealthReport& report)
{
    if (report.state == HealthState::SAFE) {
        engage(current_time, SafetyMode::SAFE_MODE, 1.0, true);
    }
    else if (mode_ != SafetyMode::SAFE_MODE) {
        if (report.state == HealthState::DEGRADED) {
            const bool actuator_fault = report.flag(FaultDomain::Actuator).raised;
            engage(current_time, SafetyMode::COMPENSATED,
                   actuator_fault ? config_.degraded_thrust_margin : 1.0, false);
        }
        else {
            engage(current_time, SafetyMode::NORMAL, 1.0, false);
        }
    }
    return command_;
}

}
