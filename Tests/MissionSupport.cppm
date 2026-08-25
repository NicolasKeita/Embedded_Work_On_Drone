/*
Filename: Tests/MissionSupport.cppm
Description: Tracking metrics, state naming and reporting helpers for autonomous flight scenarios.

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

// Composante de l'etat mesure associee a l'axe de suivi demande.
[[nodiscard]] double component_value(const AircraftState& state, TrackingAxis axis);

// Composante de la consigne cible associee a l'axe de suivi demande.
[[nodiscard]] double component_value(const sim::control::TargetState& target, TrackingAxis axis);

// Nom lisible d'un etat de mission pour la journalisation.
[[nodiscard]] std::string_view mission_state_name(sim::control::MissionState state);

/*
Affiche les indicateurs de poursuite : temps d'entree dans la tolerance,
depassement maximal rapporte a l'ecart initial, erreur en regime permanent et
erreur finale.
*/
void print_metrics_report(std::string_view label, const MissionMetrics& metrics);

/*
Verifie que les etats visites contiennent le deroulement attendu de la mission,
dans l'ordre : TAKEOFF puis CLIMB puis STATION_KEEPING puis COMPLETE.
*/
bool contains_mission_sequence(const std::vector<sim::control::MissionState>& visited);

}