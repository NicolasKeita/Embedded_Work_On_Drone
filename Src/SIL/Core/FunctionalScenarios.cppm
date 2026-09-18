/*
Filename: Src/SIL/Core/FunctionalScenarios.cppm
Description: Target-agnostic functional scenario registry. This module is the single
canonical source shared by the SIL and HIL execution layers: each entry binds a
standardised scenario ID (NOMINAL-xxx, FAULT_INJECTOR-xxx, WIND-xxx) to its
description, declarative fault identity (failure mode and parameters) and shared
mission profile (target, duration, tracking tolerance, wind disturbance and
sensor/report overrides). Execution layers must not redefine any of these
fields; they only contribute environment-specific parameters such as fault
activation timing. The execution target (SIL/HIL) is injected at runtime by the
harness, so the scenario names never encode the execution environment.
Exports:
    enum class FunctionalFamily,
    struct FunctionalScenario,
    functional_scenario_count,
    functional_scenarios(),
    find_functional_scenario(),
    nominal/fault/wind scenario counts and catalog accessors

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FunctionalScenarios;

import std;

import FlightControllerTypes;
import SilFaultScenario;

export namespace sim::test {

enum class FunctionalFamily : std::uint8_t {
    Nominal,
    FaultInjection,
    MonteCarlo,
    MonteCarloFaultInjection
};

/*
Target-agnostic description of one functional scenario: its standardised ID,
human-readable description, the declarative fault identity (failure mode and
parameters; FailureMode::NONE for nominal scenarios), whether a fault is
expected, and the family used to group the scenario in the taxonomy. The fault
activation timing (start time and duration) is target-specific and applied by
each runner.

The mission fields below the fault identity are the shared, target-independent
definition of the run: mission target, mission window (s), tracking tolerance
of the acceptance criteria and the horizontal wind disturbance. The override
fields carry a value of 0.0 when the runner/controller default applies.
*/
struct FunctionalScenario {
    std::string_view          id;
    std::string_view          description;
    FunctionalFamily          family;
    sim::sil::FailureMode     failure_mode;
    sim::sil::FaultParameters parameters;
    bool                      fault_expected;

    /* Mission target of the scenario, identical under SIL and HIL. */
    sim::control::TargetState target{};

    /* Mission window in seconds. */
    std::float64_t duration_s = 30.0;

    /* Tracking tolerance (m) applied by the acceptance criteria. */
    std::float64_t tracking_tolerance = 1.0;

    /* Station-keeping window override (s); 0.0 keeps the controller default. */
    std::float64_t station_hold_seconds = 0.0;

    /* Sensor altitude-range override (m); 0.0 keeps the sensor default. */
    std::float64_t sensor_max_altitude_m = 0.0;

    /* Human-readable report period override (s); 0.0 keeps the runner default. */
    std::float64_t report_period_s = 0.0;

    /* Horizontal wind disturbance (m/s) and optional gust period (s); 0.0 is nominal. */
    std::float64_t wind_x_mps = 0.0;
    std::float64_t wind_y_mps = 0.0;
    std::float64_t wind_gust_period_s = 0.0;
};

/* Number of scenarios held by the shared registry (compile-time constant). */
constexpr std::size_t functional_scenario_count = 11;

/* Number of nominal-mission scenarios held by the nominal catalog. */
constexpr std::size_t nominal_scenario_count = 7;

/* Number of fault-injection scenarios held by the fault catalog. */
constexpr std::size_t fault_scenario_count = 2;

/* Number of wind-disturbance scenarios held by the wind catalog. */
constexpr std::size_t wind_scenario_count = 2;

[[nodiscard]] std::span<const FunctionalScenario> functional_scenarios() noexcept;

/*
Resolves a functional scenario by its standardised ID (NOMINAL-xxx,
FAULT_INJECTOR-xxx, WIND-xxx); returns nullptr when the ID is unknown.
*/
[[nodiscard]] const FunctionalScenario* find_functional_scenario(std::string_view id) noexcept;

}

namespace sim::test {

/*
Catalog accessors shared by the FunctionalScenarios-*.cpp implementation units
(module-internal, not exported to importers). The registry assembly concatenates
these catalogs in a stable order: nominal first, then fault injection, then wind.
*/
[[nodiscard]] const std::array<FunctionalScenario, nominal_scenario_count>& nominal_scenarios() noexcept;

[[nodiscard]] const std::array<FunctionalScenario, fault_scenario_count>& fault_scenarios() noexcept;

[[nodiscard]] const std::array<FunctionalScenario, wind_scenario_count>& wind_scenarios() noexcept;

}
