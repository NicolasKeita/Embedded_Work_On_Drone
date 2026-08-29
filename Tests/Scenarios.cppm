/*
Filename: Tests/Scenarios.cppm
Description: Deterministic validation scenarios (A to F) inside sim::test::scenarios.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Scenarios;

import TestHarness;

export namespace sim::test::scenarios {

// Scenario A: the vehicle is at rest, it must stay on the ground.
void rest(TestHarness& runner, double hover_rpm);

// Scenario B: RPM above hover, vertical climb.
void climb(TestHarness& runner, double hover_rpm);

// Scenario C: climb then throttle down, return to the ground.
void descent(TestHarness& runner, double hover_rpm);

// Scenario D: positive servo mean command (+10 degrees) -> pitch > 0.
void move_x(TestHarness& runner, double hover_rpm);

// Scenario E: opposed servos (+12 / -12 degrees), pure differential -> roll > 0 without pitch.
void move_y(TestHarness& runner, double hover_rpm);

// Scenario F: positive mean (+5 degrees) and negative differential -> pitch > 0 and roll < 0.
void combined(TestHarness& runner, double hover_rpm);

} // namespace sim::test::scenarios
