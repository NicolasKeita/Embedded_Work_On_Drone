/*
Filename: Tests/FlightScenarios.cppm
Description: Autonomous axis scenarios G, H and I : altitude loop and cascaded X/Y position loops.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FlightScenarios;

import TestHarness;

export namespace sim::test::flight_scenarios {

// Scenario G : boucle d'altitude autonome, convergence vers z = 100 m avec metriques.
void autonomous_altitude(TestHarness& runner, double hover_rpm);

// Scenario H : cascade position X -> tangage -> servos, retour de x = 20 m vers x = 0.
void autonomous_position_x(TestHarness& runner, double hover_rpm);

// Scenario I : cascade position Y -> roulis -> servos, retour de y = -15 m vers y = 0.
void autonomous_position_y(TestHarness& runner, double hover_rpm);

}
