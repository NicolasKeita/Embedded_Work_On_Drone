/*
Filename: Tests/Mission/MissionSupport.cpp
Description: Implementation of tracking metrics and reporting helpers for autonomous flight scenarios.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MissionSupport;

import std;

import Aircraft;
import FlightController;

namespace sim::test {

double MissionMetrics::overshoot_percent() const noexcept
{
    return initial_gap > 0.0 ? 100.0 * overshoot_units / initial_gap : 0.0;
}

double component_value(const AircraftState& state, TrackingAxis axis)
{
    switch (axis) {
    case TrackingAxis::x_axis: return state.x;
    case TrackingAxis::y_axis: return state.y;
    case TrackingAxis::z_axis: return state.z;
    }
    return 0.0;
}

double component_value(const sim::control::TargetState& target, TrackingAxis axis)
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

bool contains_mission_sequence(const std::vector<sim::control::MissionState>& visited)
{
    constexpr std::array<sim::control::MissionState, 4> expected{
        sim::control::MissionState::TAKEOFF, sim::control::MissionState::CLIMB,
        sim::control::MissionState::STATION_KEEPING, sim::control::MissionState::COMPLETE};

    std::size_t cursor = 0;
    for (sim::control::MissionState state : visited) {
        if (cursor < expected.size() && state == expected[cursor]) {
            ++cursor;
        }
    }
    return cursor == expected.size();
}

}
