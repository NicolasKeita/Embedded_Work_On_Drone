/*
Filename: Tests/Mission/MissionRunner.cppm
Description: Autonomous mission run loop with tracking metrics and reporting helpers.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module MissionRunner;

import std;

import Aircraft;
import FlightController;

using sim::control::FlightController;

export namespace sim::test {

enum class TrackingAxis { x_axis, y_axis, z_axis };

// Fixed capacity of the visited-state trace (no dynamic allocation).
inline constexpr std::size_t kMaxVisitedStates = 16;

struct MissionMetrics {
    double initial_gap = 0.0;
    double time_within_tolerance = -1.0;
    double overshoot_units = 0.0;
    double steady_state_error = 0.0;
    double final_error = 0.0;

    [[nodiscard]] double overshoot_percent() const noexcept;
};

// Component of the measured state associated with the requested tracking axis.
[[nodiscard]] double component_value(const AircraftState& state, TrackingAxis axis);

// Component of the target setpoint associated with the requested tracking axis.
[[nodiscard]] double component_value(const sim::control::TargetState& target, TrackingAxis axis);

/*
Prints the tracking indicators: time to enter tolerance, maximum overshoot
relative to the initial gap, steady-state error and final error.
*/
void print_metrics_report(std::string_view label, const MissionMetrics& metrics);

/*
Checks that the visited states contain the expected mission flow, in order:
TAKEOFF then CLIMB then STATION_KEEPING then COMPLETE.
*/
bool contains_mission_sequence(std::span<const sim::control::MissionState> visited);

struct MissionRunRequest {
    sim::control::TargetState target;
    double duration = 0.0;
    TrackingAxis axis = TrackingAxis::z_axis;
    double tolerance = 0.0;
    bool stop_on_zone = false;
};

struct MissionRunTrace {
    std::array<sim::control::MissionState, kMaxVisitedStates> visited_states{};
    std::size_t visited_count = 0;
    MissionMetrics metrics;

    void record(sim::control::MissionState state);
};

/*
Advances an autonomous mission step by step: applies the controller, integrates
the physics, periodically logs the state, traces the state machine transitions
and accumulates tracking metrics on the requested axis. With stop_on_zone, the
loop stops as soon as the vehicle holds the target zone.
*/
MissionRunTrace run_mission(FlightController& ctrl, Aircraft& craft,
                            const MissionRunRequest& run);

}

namespace sim::test {

/*
Prints one timestamped state row of the aircraft during mission runs.
*/
void print_state_row(const Aircraft& aircraft, double timeSeconds);

/*
Executes the step-by-step simulation loop: applies the controller, integrates
the physics, periodically logs the state, traces the transitions and accumulates
the tracking metrics on the requested axis. Closes the metrics when finished.
*/
void run_control_loop(FlightController& ctrl, Aircraft& craft, const MissionRunRequest& run,
                      MissionRunTrace& trace, double direction, double target_value);

}
