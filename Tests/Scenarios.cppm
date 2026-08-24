/*
Filename: Tests/Scenarios.cppm
Description: Deterministic validation scenarios (A to F) inside sim::test::scenarios.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Scenarios;

import TestHarness;

export namespace sim::test::scenarios {

// Scenario A : l'appareil est au repos, il doit rester pose au sol.
void rest(TestHarness& runner, double hover_rpm);

// Scenario B : RPM superieur au stationnaire, montee verticale.
void climb(TestHarness& runner, double hover_rpm);

// Scenario C : montee puis reduction des gaz, retour au sol.
void descent(TestHarness& runner, double hover_rpm);

// Scenario D : consigne moyenne des servos positive (+10 degres) -> pitch > 0.
void move_x(TestHarness& runner, double hover_rpm);

// Scenario E : servos opposes (+12 / -12 degres), differentiel pur -> roll > 0 sans pitch.
void move_y(TestHarness& runner, double hover_rpm);

// Scenario F : moyenne positive (+5 degres) et differentiel negatif -> pitch > 0 et roll < 0.
void combined(TestHarness& runner, double hover_rpm);

} // namespace sim::test::scenarios
