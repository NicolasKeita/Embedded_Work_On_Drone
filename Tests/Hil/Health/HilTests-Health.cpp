/*
Filename: Tests/Hil/Health/HilTests-Health.cpp
Description: HIL FC2 health supervision tests: verifies that a new HIL run gets a complete
initial-heartbeat grace period after a supervision rearm.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import FlightControllerTypes;
import HealthMonitor;
import TestHarness;

namespace sim::test::hil {

/* Verifies that a new HIL run gets a complete initial-heartbeat grace period. */
void test_supervision_rearm(sim::test::TestHarness& runner)
{
    runner.set_context("FC2 supervision rearm");
    constexpr std::float64_t reset_time_s = 10.0;
    constexpr std::float64_t initial_timeout_s = 2.0;
    sim::safety::LinkSupervision supervision{
        .link_up = true,
        .last_heartbeat_time = -1.0,
        .monitoring_started_time = 0.0,
    };
    sim::safety::HealthMonitor monitor{sim::safety::HealthMonitorConfig{
        .initial_heartbeat_timeout_s = initial_timeout_s,
    }};

    const sim::safety::HealthReport stale_report =
        monitor.evaluate_link(reset_time_s, supervision);
    runner.check(stale_report.state == sim::safety::HealthState::SAFE,
                 "stale initial-heartbeat window has expired");

    monitor = sim::safety::HealthMonitor{sim::safety::HealthMonitorConfig{
        .initial_heartbeat_timeout_s = initial_timeout_s,
    }};
    sim::safety::rearm_link_supervision(supervision, reset_time_s);

    const sim::safety::HealthReport grace_report =
        monitor.evaluate_link(reset_time_s + initial_timeout_s, supervision);
    runner.check(grace_report.state == sim::safety::HealthState::HEALTHY,
                 "rearm grants the complete initial-heartbeat window");

    const sim::safety::HealthReport expired_report =
        monitor.evaluate_link(reset_time_s + initial_timeout_s + 0.01, supervision);
    runner.check(expired_report.state == sim::safety::HealthState::SAFE,
                 "heartbeat timeout is raised after the rearmed window");
    runner.check(expired_report.flag(sim::safety::DetectionEvent::FC1_HEARTBEAT_TIMEOUT).raised,
                 "rearmed expiry is classified as FC1 heartbeat timeout");
}

}