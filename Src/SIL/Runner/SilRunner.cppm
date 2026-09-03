/*
Filename: Src/SIL/Runner/SilRunner.cppm
Description: Public interface of the SIL orchestration engine (run context, configuration, output and error types come from SilRunnerContext).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilRunner;

import std;

import SilTypes;

export import SilRunnerContext;

export namespace sim::sil {

// SIL orchestrator: fixed-time-step loop linking Aircraft -> FaultInjector ->
// Sensors/Comms -> FC1/FC2 -> HealthMonitor -> SafetyManager -> Actuators.
// Logging is purely observational: events and telemetry never alter the run.
class SILRunner {
public:
    explicit SILRunner(SilConfig config = {});

    // Runs the scenarios and returns the structured result and trace artifacts.
    [[nodiscard]] std::expected<SilRunOutput, SilError> run(std::span<const FaultScenario> scenarios);

private:
    static std::expected<RunContext, SilError> make_context(const SilConfig&, std::span<const FaultScenario>);
    static void execute(RunContext&);
    static void apply_injectors(RunContext&);
    static void update_fc1(RunContext&);
    static void update_monitoring(RunContext&);
    static void apply_actuators(RunContext&);
    static void update_metrics(RunContext&);
    static void finalize(RunContext&);

    SilConfig config_;
};

}
