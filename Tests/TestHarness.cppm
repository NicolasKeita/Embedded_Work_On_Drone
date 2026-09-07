/*
Filename: Tests/TestHarness.cppm
Description: Encapsulated test harness for aircraft simulation scenarios. Carries
the execution target (SIL/HIL/Simulation) so every assertion pairs the scenario
ID with the target tag ([ID][TARGET]); scenarios stay target-agnostic.
Exports:
    enum class RunTarget,
    run_target_name(),
    run_target_tag(),
    parse_run_target(),
    struct HarnessConfig,
    class TestHarness

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module TestHarness;

import std;

import Aircraft;

export namespace sim::test {

/*
Execution environment of a scenario. The scenario logic is target-agnostic: the
same functional scenario ID runs under either the SIL or the HIL configuration,
and the target only selects the runtime tag emitted by the harness
([ID][SIL] or [ID][HIL]). Simulation marks the pure physics validation runs that
do not exercise the SIL/HIL runners.
*/
enum class RunTarget : std::uint8_t {
    Simulation,
    SIL,
    HIL
};

/*
Lowercase runtime name of an execution target, matching the --target=<name>
command-line argument ("simulation", "sil", "hil").
*/
[[nodiscard]] std::string_view run_target_name(RunTarget target) noexcept;

/*
Uppercase tag used to pair a scenario ID with its execution target in the logs
and reports ([NOMINAL-001][SIL], [FAULT_INJECTOR-001][HIL]).
*/
[[nodiscard]] std::string_view run_target_tag(RunTarget target) noexcept;

/*
Parses the --target=<name> argument (case-insensitive) into a RunTarget; returns
    std::errc::invalid_argument when the name is not a recognised execution target.
*/
[[nodiscard]] std::expected<RunTarget, std::errc> parse_run_target(std::string_view name) noexcept;

struct HarnessConfig {
    std::float64_t dt{0.01};
    std::size_t    log_interval_steps{100};
    RunTarget      target{RunTarget::Simulation};
    bool           verbose{true};
};

class TestHarness {
public:
    explicit TestHarness(HarnessConfig config = {}) : config_(config) {}

    [[nodiscard]] RunTarget target() const noexcept { return config_.target; }

    // Reports whether verbose per-step output (telemetry table and scenario brief) is enabled.
    [[nodiscard]] bool verbose() const noexcept { return config_.verbose; }

    // Sets the active scenario or sub-suite tag used by check() ([tag][target]).
    void set_context(std::string_view tag);

    // Sets the active scenario tag and prints a section header ([id][target] description).
    void begin_scenario(std::string_view id, std::string_view description);

    // Assertions.
    void check(bool condition, std::string_view label);
    [[nodiscard]] std::uint32_t failure_count() const noexcept { return failures_; }
    [[nodiscard]] bool passed() const noexcept { return failures_ == 0; }

    // Logging & execution.
    void log_header() const;
    void log_step(const Aircraft& aircraft) const;
    void log_step(const Aircraft& aircraft, std::float64_t time_seconds) const;
    void run(Aircraft& aircraft, std::float64_t duration_seconds);

    // Contextual helpers / scenarios.
    void take_off(Aircraft& aircraft, std::float64_t target_rpm);
    void reset();

    void record_metrics(std::float64_t overshoot, std::float64_t settling_time,
                        std::float64_t steady_state_error, std::float64_t max_acceleration);
    [[nodiscard]] std::float64_t overshoot() const noexcept { return overshoot_; }
    [[nodiscard]] std::float64_t settling_time() const noexcept { return settling_time_; }
    [[nodiscard]] std::float64_t steady_state_error() const noexcept { return steady_state_error_; }
    [[nodiscard]] std::float64_t max_acceleration() const noexcept { return max_acceleration_; }

private:
    HarnessConfig  config_;
    std::uint32_t  failures_{0};
    std::float64_t current_time_{0.0};
    std::size_t    step_count_{0};
    std::string    context_;
    std::float64_t overshoot_{0.0};
    std::float64_t settling_time_{0.0};
    std::float64_t steady_state_error_{0.0};
    std::float64_t max_acceleration_{0.0};
};

}
