/*
Filename: Src/Embedded/Hil/Runner/Context/HilRunnerTypes.cppm
Description: Error modes, mission result and structured run output of the HIL runner.
HilResult carries the mission/safety/fault outcome, the communication and timing
statistics and the final verdict; HilRunOutput bundles the result with the recorded
event trace and the dual truth/sensor telemetry streams.
Export summary: HilError, kHilMaxFaultScenarios, HilResult, HilRunOutput.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilRunnerTypes;

import std;

import FlightControllerTypes;
import HealthMonitor;
import HilConfig;
import HilEvents;
import HilTelemetry;
import HilTiming;
import HilTransport;
import SafetyManager;
import SilFaultScenario;

export namespace sim::hil {

/*
Typed failures of a HIL run (infrastructure/HIL-bench domain errors: scenario
list capacity, scenario validation, configuration; never aircraft failures).
*/
enum class HilError {
    TooManyScenarios,
    FaultScenarioRejected,
    InvalidConfiguration
};

// Maximum number of fault scenarios a single HIL run can carry (fixed capacity, matches
// the SIL baseline limit).
inline constexpr std::size_t kHilMaxFaultScenarios = 8;

struct HilResult {
    bool                       mission_success = false;
    sim::control::MissionState final_state = sim::control::MissionState::TAKEOFF;
    std::float64_t             mission_end_time = -1.0;

    sim::safety::HealthState    final_health = sim::safety::HealthState::HEALTHY;
    sim::safety::SafetyMode     final_safety_mode = sim::safety::SafetyMode::NORMAL;
    bool                        degraded_reached = false;
    bool                        compensated_reached = false;
    bool                        safe_mode_reached = false;
    sim::safety::DetectionEvent first_detection_event = sim::safety::DetectionEvent::FC1_HEARTBEAT_TIMEOUT;

    sim::sil::FailureMode failure_mode = sim::sil::FailureMode::NONE;
    bool                  fault_detected = false;
    std::float64_t        fault_injected_time = -1.0;
    std::float64_t        detection_time = -1.0;
    std::float64_t        safety_response_time = -1.0;
    std::float64_t        detection_latency = -1.0;
    std::float64_t        response_latency = -1.0;

    std::float64_t max_position_error_m = 0.0;
    std::float64_t max_altitude_error_m = 0.0;
    std::float64_t final_x_m = 0.0;
    std::float64_t final_y_m = 0.0;
    std::float64_t final_altitude_m = 0.0;
    std::float64_t max_pitch_rad = 0.0;
    std::float64_t max_roll_rad = 0.0;

    HilCommStats     comms{};
    HilTimingStats   timing{};
    bool             real_time_pacing = true;
    std::string_view scenario_id{};
    bool             fault_expected = false;
    bool             test_verdict = false;

    [[nodiscard]] bool compute_verdict(bool fault_expected) const noexcept;
};

struct HilRunOutput {
    HilResult                    result{};
    HilConfig                    config{};
    std::vector<HilEvent>        events{};
    std::vector<HilSensorSample> telemetry{};
    std::vector<HilTruthSample>  ground_truth{};
};

}
