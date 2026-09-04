/*
Filename: Src/SIL/Validation/FlightControlValidation.cppm
Description: Monte-Carlo validation subsystem for the SIL engine.
Exports: FailureReason, Scenario, SimulationResult, CampaignSummary, ScenarioGenerator, ResultCollector, MonteCarloRunner, failure_reason_name/id.
Copyright (c) 2026 Nicolas K.
All rights reserved.
*/
export module FlightControlValidation;
import std;
import SilRunner;
import SilTypes;
export namespace sim::sil::validation {
enum class FailureReason : std::uint8_t {
    None                 = 0,
    MissionAborted       = 1,
    MissionFailed        = 2,
    FaultUndetected      = 3,
    WatchdogMissed       = 4,
    SafetyModeNotReached = 5,
    PositionExceeded     = 6,
    AltitudeExceeded     = 7,
    CommsTimeout         = 8,
    RunnerError          = 9
};
struct Scenario {
    std::uint64_t          run_id = 0;
    std::uint64_t          scenario_seed = 0;
    FaultType              fault_type = FaultType::None;
    FaultTarget            fault_target = FaultTarget::Unspecified;
    FaultProfile           fault_profile = FaultProfile::Permanent;
    std::float64_t         fault_start_time_s = 0.0;
    std::float64_t         fault_duration_s = 0.0;
    std::float64_t         fault_loss_probability = 0.0;
    SensorCorruptionMode   sensor_corruption = SensorCorruptionMode::None;
    std::float64_t         corrupted_altitude_m = 0.0;
    std::float64_t         actuator_efficiency = 1.0;
    std::float64_t         wind_x_mps = 0.0;
    std::float64_t         wind_y_mps = 0.0;
    std::float64_t         ambient_pressure_kpa = 101.325;
    std::float64_t         ambient_temperature_k = 288.15;
    std::float64_t         sensor_noise_altitude_m = 0.0;
    std::float64_t         sensor_noise_position_m = 0.0;
    std::float64_t         sensor_dropout_rate = 0.0;
    std::float64_t         comms_loss_probability = 0.0;
    std::float64_t         comms_transport_latency_s = 0.004;
    bool                   comms_link_up = true;
    std::float64_t         initial_altitude_m = 0.0;
    std::float64_t         initial_x_m = 0.0;
    std::float64_t         initial_y_m = 0.0;
    std::float64_t         target_altitude_m = 10.0;
    std::float64_t         simulation_duration_s = 30.0;
    std::float64_t         time_step_s = 0.01;
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
    std::uint64_t total_runs = 0;
    std::uint64_t successful_runs = 0;
    std::uint64_t failed_runs = 0;
    std::uint64_t runner_errors = 0;
    std::float64_t success_rate = 0.0;
    std::float64_t mean_detection_latency_s = 0.0;
    std::float64_t mean_response_latency_s = 0.0;
    std::float64_t mean_position_error_m = 0.0;
    std::float64_t mean_altitude_error_m = 0.0;
    std::array<std::uint64_t, 10> failure_counts{};
};
class ScenarioGenerator {
public:
    explicit ScenarioGenerator(std::uint64_t master_seed);
    [[nodiscard]] Scenario generate_scenario(std::uint64_t run_id);
    [[nodiscard]] std::uint64_t master_seed() const noexcept;
private:
    static std::uint64_t mix_seed(std::uint64_t master_seed, std::uint64_t run_id);
    std::uint64_t master_seed_;
};
class ResultCollector {
public:
    ResultCollector() = default;
    void record(SimulationResult result);
    void reserve(std::size_t count);
    [[nodiscard]] std::span<const SimulationResult> results() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] CampaignSummary summarize() const;
    void write_csv(std::ostream& out) const;
    void clear() noexcept;
private:
    std::vector<SimulationResult> results_;
};
class MonteCarloRunner {
public:
    MonteCarloRunner(std::uint64_t master_seed, SilConfig config = {});
    [[nodiscard]] CampaignSummary run(std::uint64_t run_count);
    [[nodiscard]] const ResultCollector& collector() const noexcept;
    [[nodiscard]] ResultCollector& collector() noexcept;
    [[nodiscard]] std::uint64_t master_seed() const noexcept;
private:
    static SimulationResult classify(const Scenario& scenario,
                                      const sim::sil::SimulationResult& sil_result);
    void execute_run(std::uint64_t run_id);
    ScenarioGenerator generator_;
    SilConfig config_;
    ResultCollector collector_;
};
[[nodiscard]] std::string_view failure_reason_name(FailureReason reason);
[[nodiscard]] std::uint32_t failure_reason_id(FailureReason reason);
void sample_fault_window(Scenario& scenario, std::mt19937_64& generator);
void sample_fault_parameters(Scenario& scenario, std::mt19937_64& generator);
void sample_environment(Scenario& scenario, std::mt19937_64& generator);
void sample_initial_conditions(Scenario& scenario, std::mt19937_64& generator);
}
