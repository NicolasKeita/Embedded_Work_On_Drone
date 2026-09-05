/*
Filename: Tests/Mission/MissionRunner-Support.cpp
Description: Tracking metrics and reporting helpers for autonomous flight scenarios.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MissionRunner;

import std;

import Aircraft;
import FlightController;

namespace sim::test {

/*
Prints one timestamped state row of the aircraft during mission runs.
*/
void print_state_row(const Aircraft& aircraft, std::float64_t timeSeconds)
{
    const AircraftState& s = aircraft.state();

    std::cout << std::fixed << std::setw(9) << std::setprecision(2) << timeSeconds;
    std::cout << "   " << std::setprecision(3) << std::setw(11) << s.x
              << std::setw(11) << s.y << std::setw(11) << s.z;
    std::cout << "   " << std::setw(11) << s.vx << std::setw(11) << s.vy
              << std::setw(11) << s.vz;
    std::cout << "   " << std::setprecision(2) << std::setw(11) << s.pitch * 180.0 / std::numbers::pi
              << std::setw(11) << s.roll * 180.0 / std::numbers::pi;
    std::cout << "   " << std::setprecision(0) << std::setw(11) << s.actual_rpm
              << std::defaultfloat << std::endl;
}

std::float64_t MissionMetrics::overshoot_percent() const noexcept
{
    return initial_gap > 0.0 ? 100.0 * overshoot_units / initial_gap : 0.0;
}

std::float64_t component_value(const AircraftState& state, TrackingAxis axis)
{
    switch (axis) {
    case TrackingAxis::x_axis: return state.x;
    case TrackingAxis::y_axis: return state.y;
    case TrackingAxis::z_axis: return state.z;
    }
    return 0.0;
}

std::float64_t component_value(const sim::control::TargetState& target, TrackingAxis axis)
{
    switch (axis) {
    case TrackingAxis::x_axis: return target.x;
    case TrackingAxis::y_axis: return target.y;
    case TrackingAxis::z_axis: return target.z;
    }
    return 0.0;
}

void print_metrics_report(std::string_view label, const MissionMetrics& metrics)
{
    std::cout << "  Metriques de poursuite (" << label << ") :" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "    ecart initial             : " << std::setw(9) << metrics.initial_gap
              << " m" << std::endl;
    std::cout << "    temps entree tolerance    : " << std::setw(9)
              << metrics.time_within_tolerance << " s" << std::endl;
    std::cout << "    depassement maximal       : " << std::setw(9) << metrics.overshoot_units
              << " m (" << metrics.overshoot_percent() << " %)" << std::endl;
    std::cout << "    erreur regime permanent   : " << std::setw(9)
              << metrics.steady_state_error << " m" << std::endl;
    std::cout << "    erreur finale             : " << std::setw(9) << metrics.final_error
              << " m" << std::endl;
    std::cout << std::defaultfloat;
}

/*
Records a visited mission state in the fixed-capacity trace; states beyond the
capacity are dropped (harness mission flows never exceed it).
*/
void MissionRunTrace::record(sim::control::MissionState state)
{
    if (visited_count < visited_states.size()) {
        visited_states[visited_count] = state;
        ++visited_count;
    }
}

bool contains_mission_sequence(std::span<const sim::control::MissionState> visited)
{
    constexpr std::array<sim::control::MissionState, 4> expected{
        sim::control::MissionState::TAKEOFF, sim::control::MissionState::CLIMB,
        sim::control::MissionState::STATION_KEEPING, sim::control::MissionState::COMPLETE};
    std::size_t                                         cursor = 0;

    for (sim::control::MissionState state : visited) {
        if (cursor < expected.size() && state == expected[cursor]) {
            ++cursor;
        }
    }
    return cursor == expected.size();
}

}
