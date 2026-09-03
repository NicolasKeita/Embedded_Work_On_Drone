/*
Filename: Src/Safety/HealthMonitor.cppm
Description: FC2 health monitor : heartbeat/comms timeouts, sensor validation, actuator mismatch.
Exports:
    enum class HealthState,
    enum class FaultDomain,
    struct FaultFlag,
    struct HealthReport,
    struct HealthMonitorConfig,
    class HealthMonitor,
    fault_domain_name(),
    health_state_name()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HealthMonitor;

import std;

import Aircraft;
import CommsBus;
import Telemetry;

export namespace sim::safety {

enum class HealthState { HEALTHY, DEGRADED, SAFE, FAILED };

enum class FaultDomain { FC1Heartbeat, Communication, Sensor, Actuator };

struct FaultFlag {
    bool           raised = false;
    std::float64_t raised_time = -1.0;
};

struct HealthReport {
    HealthState              state = HealthState::HEALTHY;
    std::array<FaultFlag, 4> flags{};

    [[nodiscard]] const FaultFlag& flag(FaultDomain domain) const;

    [[nodiscard]] std::float64_t first_detection_time() const;

    [[nodiscard]] FaultDomain first_fault_domain() const;
};

struct HealthMonitorConfig {
    std::float64_t                   heartbeat_timeout_s = 0.10;
    std::float64_t                   actuator_mismatch_rpm = 60.0;
    std::float64_t                   actuator_mismatch_hold_s = 0.50;
    sim::sil::SensorValidationLimits sensor_limits{};
};

class HealthMonitor {
public:
    explicit HealthMonitor(HealthMonitorConfig config = {});

    /*
    FC2-side health evaluation: heartbeat/comms timeout, sensor validation and
    commanded/actual actuator mismatch. Detection is agnostic: it knows nothing
    about fault injectors.
    */
    [[nodiscard]] HealthReport evaluate(std::float64_t current_time, const sim::sil::CommsBus& comms,
                                        const sim::sil::SensorTelemetry& telemetry, std::float64_t commanded_rpm);

    [[nodiscard]] HealthState state() const noexcept;

private:
    void raise(FaultDomain domain, std::float64_t time);

    void update_comms_flags(std::float64_t current_time, const sim::sil::CommsBus& comms);

    void update_sensor_flags(std::float64_t current_time, const sim::sil::SensorTelemetry& telemetry);

    void update_actuator_flags(std::float64_t current_time, const sim::sil::SensorTelemetry& telemetry,
                               std::float64_t commanded_rpm);

    [[nodiscard]] static std::size_t domain_index(FaultDomain domain);

    [[nodiscard]] static HealthState compute_state(const std::array<FaultFlag, 4>& flags);

    HealthMonitorConfig      config_;
    HealthState              state_ = HealthState::HEALTHY;
    std::array<FaultFlag, 4> flags_{};
    std::float64_t           mismatch_since_ = -1.0;
};

[[nodiscard]] std::string_view fault_domain_name(FaultDomain domain);
[[nodiscard]] std::string_view health_state_name(HealthState state);

}
