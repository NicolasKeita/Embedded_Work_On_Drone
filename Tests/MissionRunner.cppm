/*
Filename: Tests/MissionRunner.cppm
Description: Step-by-step autonomous mission run loop producing tracking metrics.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module MissionRunner;

import std;

import Aircraft;
import FlightController;
import MissionSupport;

using sim::control::FlightController;

export namespace sim::test {

struct MissionRunRequest {
    sim::control::TargetState target;
    double duration = 0.0;
    TrackingAxis axis = TrackingAxis::z_axis;
    double tolerance = 0.0;
    bool stop_on_zone = false;
};

struct MissionRunTrace {
    std::vector<sim::control::MissionState> visited_states;
    MissionMetrics metrics;
};

/*
Avance une mission autonome pas a pas : applique le controleur, integre la
physique, journalise periodiquement l'etat, trace les transitions de la machine
a etats et accumule les metriques de poursuite sur l'axe demande. Avec
stop_on_zone, la boucle s'arrete des que l'appareil tient la zone cible.
*/
MissionRunTrace run_mission(FlightController& ctrl, Aircraft& craft,
                            const MissionRunRequest& run);

}