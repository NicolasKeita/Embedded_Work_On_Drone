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

std::string_view safety_action_name(SafetyAction action)
{
    switch (action) {
    case SafetyAction::RESUME_NORMAL:
        return "RESUME_NORMAL";
    case SafetyAction::ENTER_COMPENSATED:
        return "ENTER_COMPENSATED";
    case SafetyAction::ENTER_SAFE_MODE:
        return "ENTER_SAFE_MODE";
    }
    return "UNKNOWN";
}

SafetyAction safety_action_for(SafetyMode mode) noexcept
{
    switch (mode) {
    case SafetyMode::SAFE_MODE:
        return SafetyAction::ENTER_SAFE_MODE;
    case SafetyMode::COMPENSATED:
        return SafetyAction::ENTER_COMPENSATED;
    case SafetyMode::NORMAL:
        break;
    }
    return SafetyAction::RESUME_NORMAL;
}

SafetyManager::SafetyManager(SafetyManagerConfig config) : config_{config} {}

SafetyMode SafetyManager::mode() const noexcept
{
    return mode_;
}

std::float64_t SafetyManager::response_time() const noexcept
{
    return response_time_;
}

void SafetyManager::engage(std::float64_t current_time, SafetyMode mode, std::float64_t margin, bool abort)
{
    if (mode != SafetyMode::NORMAL && response_time_ < 0.0) {
        response_time_ = current_time;
    }
    mode_ = mode;
    command_ = SafetyCommand{.mission_abort = abort, .thrust_margin = margin};
}

/*
Safety transitions: SAFE -> ENTER_SAFE_MODE (mission abort, conservative),
DEGRADED -> ENTER_COMPENSATED with a thrust margin only on proven actuator
mismatch (otherwise the detection is a sensor one and thrust must stay
nominal), HEALTHY -> RESUME_NORMAL. The SAFE_MODE transition is irreversible.
*/
SafetyCommand SafetyManager::update(std::float64_t current_time, const HealthReport& report)
{
    if (report.state == HealthState::SAFE) {
        engage(current_time, SafetyMode::SAFE_MODE, 1.0, true);
    }
    else if (mode_ != SafetyMode::SAFE_MODE) {
        if (report.state == HealthState::DEGRADED) {
            const bool actuator_detected = report.flag(DetectionEvent::ACTUATOR_MISMATCH).raised;
            const std::float64_t margin = actuator_detected ? config_.degraded_thrust_margin : 1.0;
            engage(current_time, SafetyMode::COMPENSATED, margin, false);
        }
        else {
            engage(current_time, SafetyMode::NORMAL, 1.0, false);
        }
    }
    return command_;
}

}
