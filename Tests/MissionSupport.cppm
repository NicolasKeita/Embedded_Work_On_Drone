/*
Filename: Tests/MissionSupport.cppm
Description: Tracking metrics and reporting helpers for autonomous flight scenarios.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module MissionSupport;

import std;

import Aircraft;
import FlightController;

export namespace sim::test {

enum class TrackingAxis { x_axis, y_axis, z_axis };

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
bool contains_mission_sequence(const std::vector<sim::control::MissionState>& visited);

}
