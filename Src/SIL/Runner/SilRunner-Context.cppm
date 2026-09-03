/*
Filename: Src/SIL/Runner/SilRunner-Context.cppm
Description: Interface partition of the SIL runner: run configuration, structured output and error modes.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilRunner:Context;

import std;

import Aircraft;
import FlightController;
import SilEvents;
import SilTelemetry;
import SilTypes;
import Telemetry;

export namespace sim::sil {

// Typed failures of a SIL run (no exception is ever thrown).
enum class SilError { TooManyScenarios, FaultScenarioRejected };

struct SilConfig {
    std::float64_t                 dt = 0.01;
    std::float64_t                 duration_s = 60.0;
    sim::control::TargetState      target{.z = 10.0};
    sim::control::ControllerConfig controller{.hover_rpm = Aircraft{}.hover_rpm()};
    std::float64_t                 heartbeat_timeout_s = 0.10;
    std::float64_t                 actuator_mismatch_rpm = 60.0;
    std::float64_t                 thrust_compensation_margin = 1.7;
    std::float64_t                 safe_descent_rpm_rate = 4000.0;
    SensorValidationLimits         sensor_limits{};
    std::uint64_t                  seed = 42;
    std::float64_t                 transport_latency_s = 0.004;
    std::float64_t                 telemetry_rate_hz = 20.0;
    SilLogLevel                    trace_level = SilLogLevel::Info;
};

// Full observable output of one SIL run: aggregates plus trace artifacts.
struct SilRunOutput {
    SimulationResult             result{};
    std::vector<SilEvent>        events{};
    std::vector<TelemetrySample> telemetry{};
    std::vector<TrueStateSample> ground_truth{};
};

}
