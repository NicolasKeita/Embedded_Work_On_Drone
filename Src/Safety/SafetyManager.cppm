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
    bool mission_abort = false;
    double thrust_margin = 1.0;
};

struct SafetyManagerConfig {
    double degraded_thrust_margin = 1.7;
};

class SafetyManager {
public:
    explicit SafetyManager(SafetyManagerConfig config = {});

    [[nodiscard]] SafetyCommand update(double current_time, const HealthReport& report);

    [[nodiscard]] SafetyMode mode() const noexcept;

    [[nodiscard]] double response_time() const noexcept;

private:
    void engage(double current_time, SafetyMode mode, double margin, bool abort);

    SafetyManagerConfig config_;
    SafetyMode mode_ = SafetyMode::NORMAL;
    SafetyCommand command_{};
    double response_time_ = -1.0;
};

[[nodiscard]] std::string_view safety_mode_name(SafetyMode mode);

}
