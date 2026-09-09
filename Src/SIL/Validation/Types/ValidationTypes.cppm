/*
Filename: Src/SIL/Validation/Types/ValidationTypes.cppm
Description: Validation vocabulary of the Monte-Carlo subsystem : failure reasons, scenario,
result and campaign summary types, re-exported through FlightControlValidation.
Exports:
    enum class FailureReason,
    struct Scenario,
    struct SimulationResult,
    struct CampaignSummary,
    failure_reason_name(),
    failure_reason_id()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module ValidationTypes;

import std;

import SilTypes;
import Telemetry;

export namespace sim::sil::validation {

enum class FailureReason : std::uint8_t {
    None                 = 0,
    MissionAborted       = 1,
    MissionFailed        = 2,
    FaultUndetected      = 3,
    SupervisionMissed    = 4,
    SafetyModeNotReached = 5,
    PositionExceeded     = 6,
    AltitudeExceeded     = 7,
    CommsTimeout         = 8,
    RunnerError          = 9
};

struct Scenario {
    std::uint64_t        run_id = 0;
    std::uint64_t        scenario_seed = 0;
    FailureMode          failure_mode = FailureMode::NONE;
    FaultTarget          fault_target = FaultTarget::Unspecified;
    FaultProfile         fault_profile = FaultProfile::Permanent;
    std::float64_t       fault_start_time_s = 0.0;
    std::float64_t       fault_duration_s = 0.0;
    std::float64_t       fault_loss_probability = 0.0;
    SensorCorruptionMode sensor_corruption = SensorCorruptionMode::None;
    std::float64_t       corrupted_altitude_m = 0.0;
    std::float64_t       actuator_efficiency = 1.0;
    std::float64_t       wind_x_mps = 0.0;
    std::float64_t       wind_y_mps = 0.0;
    std::float64_t       ambient_pressure_kpa = 101.325;
    std::float64_t       ambient_temperature_k = 288.15;
    std::float64_t       sensor_noise_altitude_m = 0.0;
    std::float64_t       sensor_noise_position_m = 0.0;
    std::float64_t       sensor_dropout_rate = 0.0;
    std::float64_t       comms_loss_probability = 0.0;
    std::float64_t       comms_transport_latency_s = 0.004;
    bool                 comms_link_up = true;
    std::float64_t       initial_altitude_m = 0.0;
    std::float64_t       initial_x_m = 0.0;
    std::float64_t       initial_y_m = 0.0;
    std::float64_t       target_altitude_m = 10.0;
    std::float64_t       simulation_duration_s = 30.0;
    std::float64_t       time_step_s = 0.01;
    [[nodiscard]] FaultScenario to_fault_scenario() const;
};

struct SimulationResult {
    std::uint64_t              run_id = 0;
    std::uint64_t              scenario_seed = 0;
    Scenario                   scenario{};
    sim::sil::SimulationResult sil_result{};
    FailureReason              failure_reason = FailureReason::None;
    bool                       verdict = false;
    std::float64_t             position_error_m = 0.0;
    std::float64_t             altitude_error_m = 0.0;
};

struct CampaignSummary {
    std::uint64_t                 total_runs = 0;
    std::uint64_t                 successful_runs = 0;
    std::uint64_t                 failed_runs = 0;
    std::uint64_t                 runner_errors = 0;
    std::float64_t                success_rate = 0.0;
    std::float64_t                mean_detection_latency_s = 0.0;
    std::float64_t                mean_response_latency_s = 0.0;
    std::float64_t                mean_position_error_m = 0.0;
    std::float64_t                mean_altitude_error_m = 0.0;
    std::array<std::uint64_t, 10> failure_counts{};
};

/*
Canonical human-readable label of a failure reason. The labels are stable
literals so that CSV consumers and the report generator can reference them by
string_view without copying. None is exported as an empty view so that
successful runs show a blank reason column.
*/
[[nodiscard]] std::string_view failure_reason_name(FailureReason reason);

/*
Numeric identifier of a failure reason as a 32-bit value. It mirrors the enum
underlying value but is returned through a widening conversion so that the CSV
writer never has to cast again.
*/
[[nodiscard]] std::uint32_t failure_reason_id(FailureReason reason);

}
