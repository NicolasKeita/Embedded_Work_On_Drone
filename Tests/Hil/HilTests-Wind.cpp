/*
Filename: Tests/Hil/HilTests-Wind.cpp
Description: HIL wind disturbance tests: wind catalog configuration, wind timing, direction,
ground gating and unchanged calm dynamics.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import Aircraft;
import HilConfig;
import TestHarness;

namespace sim::test::hil {

/* Checks wind timing, direction, ground gating and unchanged calm dynamics. */
void test_wind(sim::test::TestHarness& runner)
{
    runner.set_context("WIND");
    const sim::hil::HilConfig steady = test_config("WIND-001", 30.0);
    const sim::hil::HilConfig gust = test_config("WIND-002", 30.0);
    runner.check(steady.wind_y_mps == 4.0, "steady wind catalog configuration");
    runner.check(sim::hil::wind_factor(steady, 7.99) == 0.0, "calm before wind");
    runner.check(sim::hil::wind_factor(steady, 8.0) == 1.0, "wind starts at 8 s");
    runner.check(sim::hil::wind_factor(steady, 24.0) == 0.0, "wind stops at 24 s");
    runner.check(sim::hil::wind_factor(gust, 8.0) == 0.0, "gust starts smoothly");
    runner.check(sim::hil::wind_factor(gust, 10.0) == 1.0, "gust reaches peak");
    runner.check(sim::hil::wind_factor(gust, 12.0) == 0.0, "gust repeats");
    Aircraft calm;
    Aircraft windy;
    windy.set_wind(0.0, 4.0);
    windy.update(0.01);
    runner.check(windy.state().vy == 0.0, "wind does not slide grounded aircraft");
    const ControlCommand climb{.wing_rpm = calm.hover_rpm() * 1.1};
    calm.set_command(climb);
    windy.set_command(climb);
    for (std::uint32_t step = 0; step < 100; ++step) {
        calm.update(0.01);
        windy.update(0.01);
    }
    runner.check(windy.state().y > calm.state().y, "wind physically displaces airborne aircraft");
    runner.check(windy.state().x == calm.state().x, "crosswind preserves perpendicular axis");
    runner.check(windy.state().z == calm.state().z, "horizontal wind preserves vertical dynamics");
}

}
