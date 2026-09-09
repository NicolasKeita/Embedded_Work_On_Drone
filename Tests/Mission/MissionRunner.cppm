/*
Filename: Tests/Mission/MissionRunner.cppm
Description: Autonomous mission run loop with tracking metrics and reporting helpers.
Exports:
    enum class TrackingAxis,
    struct MissionMetrics,
    struct MissionRunRequest,
    struct MissionRunTrace,
    component_value(),
    print_metrics_report(),
    contains_mission_sequence(),
    run_mission()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module MissionRunner;

import std;

import Aircraft;
import FlightController;
import PhysicsDispersion;

using sim::control::FlightController;

export namespace sim::test {

enum class TrackingAxis { x_axis, y_axis, z_axis };

// Fixed capacity of the visited-state trace (no dynamic allocation).
inline constexpr std::size_t kMaxVisitedStates = 16;

struct MissionMetrics {
    std::float64_t initial_gap = 0.0;
    std::float64_t time_within_tolerance = -1.0;
    std::float64_t overshoot_units = 0.0;
    std::float64_t steady_state_error = 0.0;
    std::float64_t final_error = 0.0;
    std::float64_t max_acceleration = 0.0;

    [[nodiscard]] std::float64_t overshoot_percent() const noexcept;
};

// Component of the measured state associated with the requested tracking axis.
[[nodiscard]] std::float64_t component_value(const AircraftState& state, TrackingAxis axis);

// Component of the target setpoint associated with the requested tracking axis.
[[nodiscard]] std::float64_t component_value(const sim::control::TargetState& target, TrackingAxis axis);

/*
Prints the tracking indicators: time to enter tolerance, maximum overshoot
relative to the initial gap, steady-state error and final error.
*/
void print_metrics_report(std::string_view label, const MissionMetrics& metrics);

/*
Checks that the visited states contain the expected mission flow, in order:
SPIN_UP then TAKEOFF, CLIMB, STATION_KEEPING and COMPLETE.
*/
bool contains_mission_sequence(std::span<const sim::control::MissionState> visited);

struct MissionRunRequest {
    sim::control::TargetState target;
    std::float64_t            duration = 0.0;
    TrackingAxis              axis = TrackingAxis::z_axis;
    std::float64_t            tolerance = 0.0;
    bool                      stop_on_zone = false;
    bool                      verbose = true;
};

struct MissionRunTrace {
    std::array<sim::control::MissionState, kMaxVisitedStates> visited_states{};
    std::size_t                                               visited_count = 0;
    MissionMetrics                                            metrics;

    void record(sim::control::MissionState state);
};

/*
Advances an autonomous mission step by step: applies the controller, integrates
the physics, periodically logs the state, traces the state machine transitions
and accumulates tracking metrics on the requested axis. With stop_on_zone, the
loop stops as soon as the vehicle holds the target zone.
*/
MissionRunTrace run_mission(FlightController& ctrl, Aircraft& craft,
                            const MissionRunRequest& run,
                            const sim::PhysicsDispersion& dispersion = {});

}

namespace sim::test {

/*
Prints one timestamped state row of the aircraft during mission runs.
*/
void print_state_row(const Aircraft& aircraft, std::float64_t timeSeconds);

/* Records a state machine transition in the trace and reports it on the console. */
void log_state_transition(MissionRunTrace& trace, sim::control::MissionState& previous,
                          sim::control::MissionState current, std::float64_t time, bool verbose);

/* Reports whether the vehicle holds the target zone and the run should stop there. */
bool zone_reached(const MissionRunRequest& run, sim::control::MissionState state,
                  const AircraftState& s);

/*
Executes the step-by-step simulation loop: applies the controller, integrates
the physics, periodically logs the state, traces the transitions and accumulates
the tracking metrics on the requested axis. Closes the metrics when finished.
*/
void run_control_loop(FlightController& ctrl, Aircraft& craft, const MissionRunRequest& run,
                      MissionRunTrace& trace, std::float64_t direction, std::float64_t target_value);

}
