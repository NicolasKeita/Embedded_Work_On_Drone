/*
Filename: Tests/Scenarios.cppm
Description: Public interface of the deterministic validation scenarios (A to F).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Scenarios;

// Scenario A : l'appareil est au repos, il doit rester pose au sol.
export void ScenarioRest(double hoverRpm);

// Scenario B : RPM superieur au stationnaire, montee verticale.
export void ScenarioClimb(double hoverRpm);

// Scenario C : montee puis reduction des gaz, retour au sol.
export void ScenarioDescent(double hoverRpm);

// Scenario D : consigne moyenne des servos positive (+10 degres) -> pitch > 0.
export void ScenarioMoveX(double hoverRpm);

// Scenario E : servos opposes (+12 / -12 degres), differentiel pur -> roll > 0 sans pitch.
export void ScenarioMoveY(double hoverRpm);

// Scenario F : moyenne positive (+5 degres) et differentiel negatif -> pitch > 0 et roll < 0.
export void ScenarioCombined(double hoverRpm);