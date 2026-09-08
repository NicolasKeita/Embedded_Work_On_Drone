/*
Filename: Src/Safety/SafetyManager.cppm
Description: Safety manager : safety modes, safety actions, mission abort and thrust compensation commands.
Exports:
    enum class SafetyMode,
    enum class SafetyAction,
    struct SafetyCommand,
    struct SafetyManagerConfig,
    class SafetyManager,
    safety_mode_name(),
    safety_action_name(),
    safety_action_for()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SafetyManager;

import std;

import HealthMonitor;

export namespace sim::safety {

/*
Safety mode of the system (system state). NORMAL: nominal operation;
COMPENSATED: degraded mode with a thrust margin; SAFE_MODE: conservative safe
state with mission abort (irreversible).
*/
enum class SafetyMode { NORMAL, COMPENSATED, SAFE_MODE };

/*
Safety action commanded in reaction to a diagnosis (a reaction, not a state):
the transition the SafetyManager orders when it engages a safety mode.
*/
enum class SafetyAction { RESUME_NORMAL, ENTER_COMPENSATED, ENTER_SAFE_MODE };

struct SafetyCommand {
    bool           mission_abort = false;
    std::float64_t thrust_margin = 1.0;
};

struct SafetyManagerConfig {
    std::float64_t degraded_thrust_margin = 1.7;
};

class SafetyManager {
public:
    explicit SafetyManager(SafetyManagerConfig config = {});

    [[nodiscard]] SafetyCommand update(std::float64_t current_time, const HealthReport& report);

    [[nodiscard]] SafetyMode mode() const noexcept;

    [[nodiscard]] std::float64_t response_time() const noexcept;

private:
    void engage(std::float64_t current_time, SafetyMode mode, std::float64_t margin, bool abort);

    SafetyManagerConfig config_;
    SafetyMode          mode_ = SafetyMode::NORMAL;
    SafetyCommand       command_{};
    std::float64_t      response_time_ = -1.0;
};

[[nodiscard]] std::string_view safety_mode_name(SafetyMode mode);
[[nodiscard]] std::string_view safety_action_name(SafetyAction action);

/* Safety action associated with engaging a safety mode. */
[[nodiscard]] constexpr SafetyAction safety_action_for(SafetyMode mode) noexcept
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

}
