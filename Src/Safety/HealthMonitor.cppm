/*
Filename: Src/Safety/HealthMonitor.cppm
Description: FC2 health monitor : heartbeat and comms supervision, sensor validation.
Exports: HealthState, DetectionEvent, DetectionFlag, HealthReport, HealthMonitorConfig,
    HealthMonitor, kDetectionEventCount, detection_event_name(), health_state_name()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HealthMonitor;

import std;

import Aircraft;
import CommsBus;
import Telemetry;

export namespace sim::safety {

/*
Global system health state (a state, not a failure mode). FAILED was removed as a
dead state no detection path ever produced.
*/
enum class HealthState { HEALTHY, DEGRADED, SAFE };

/*
Detection events raised by the FC2 supervision (detection outputs, never failure
modes): FC1_HEARTBEAT_TIMEOUT (remote FC1 liveness supervision while the link
stays up), COMMUNICATION_TIMEOUT (FC1-FC2 link supervision),
SENSOR_VALIDATION_FAILED (range/NaN validation of the sensor telemetry) and
ACTUATOR_MISMATCH (sustained commanded/actual RPM mismatch).
*/
enum class DetectionEvent { FC1_HEARTBEAT_TIMEOUT, COMMUNICATION_TIMEOUT, SENSOR_VALIDATION_FAILED, ACTUATOR_MISMATCH };

// Number of detection events supervised by the health monitor (flag array size).
inline constexpr std::size_t kDetectionEventCount = 4;

struct DetectionFlag {
    bool           raised = false;
    std::float64_t raised_time = -1.0;
};

struct HealthReport {
    HealthState                                     state = HealthState::HEALTHY;
    std::array<DetectionFlag, kDetectionEventCount> flags{};

    [[nodiscard]] const DetectionFlag& flag(DetectionEvent event) const;

    [[nodiscard]] std::float64_t first_detection_time() const;

    [[nodiscard]] DetectionEvent first_detection_event() const;
};

struct HealthMonitorConfig {
    std::float64_t                   heartbeat_timeout_s = 0.30;
    std::float64_t                   initial_heartbeat_timeout_s = 2.0;
    std::float64_t                   actuator_mismatch_rpm = 60.0;
    std::float64_t                   actuator_mismatch_hold_s = 0.50;
    sim::sil::SensorValidationLimits sensor_limits{};
};

struct LinkSupervision {
    bool           link_up = true;
    std::float64_t last_heartbeat_time = -1.0;
    std::float64_t monitoring_started_time = -1.0;
};

class HealthMonitor {
public:
    explicit HealthMonitor(HealthMonitorConfig config = {});

    /*
    FC2-side health evaluation: heartbeat/comms supervision, sensor validation
    and commanded/actual actuator mismatch. Detection is agnostic: it knows
    nothing about fault injectors.
    */
    [[nodiscard]] HealthReport evaluate(std::float64_t current_time, const sim::sil::CommsBus& comms,
                                        const sim::sil::SensorTelemetry& telemetry, std::float64_t commanded_rpm);

    [[nodiscard]] HealthReport evaluate(std::float64_t current_time, const LinkSupervision& supervision,
                                        const sim::sil::SensorTelemetry& telemetry, std::float64_t commanded_rpm);

    /* Evaluates only FC1/link liveness while preserving any other latched detections. */
    [[nodiscard]] HealthReport evaluate_link(std::float64_t current_time, const LinkSupervision& supervision);

    [[nodiscard]] HealthState state() const noexcept;

private:
    void raise(DetectionEvent event, std::float64_t time);

    void update_comms_flags(std::float64_t current_time, const sim::sil::CommsBus& comms);

    void update_comms_flags(std::float64_t current_time, const LinkSupervision& supervision);

    void update_sensor_flags(std::float64_t current_time, const sim::sil::SensorTelemetry& telemetry);

    void update_actuator_flags(std::float64_t current_time, const sim::sil::SensorTelemetry& telemetry,
                               std::float64_t commanded_rpm);

    [[nodiscard]] static std::size_t detection_index(DetectionEvent event);

    [[nodiscard]] static HealthState compute_state(const std::array<DetectionFlag, kDetectionEventCount>& flags);

    HealthMonitorConfig                             config_;
    HealthState                                     state_ = HealthState::HEALTHY;
    std::array<DetectionFlag, kDetectionEventCount> flags_{};
    std::float64_t                                  mismatch_since_ = -1.0;
};

[[nodiscard]] std::string_view detection_event_name(DetectionEvent event);
[[nodiscard]] std::string_view health_state_name(HealthState state);

}
