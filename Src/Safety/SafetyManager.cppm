/*
Filename: Src/Safety/SafetyManager.cppm
Description: Safety manager : safety modes, mission abort and thrust compensation commands.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SafetyManager;

import std;

import HealthMonitor;

export namespace sim::safety {

enum class SafetyMode { NORMAL, COMPENSATED, SAFE_MODE };

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

}
