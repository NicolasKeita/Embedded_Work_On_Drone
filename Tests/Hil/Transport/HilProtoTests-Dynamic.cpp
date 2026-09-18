/*
Filename: Tests/Hil/Transport/HilProtoTests-Dynamic.cpp
Description: HIL dynamic control setpoint tests: changes the wire setpoint during one
control session without resetting the FC, driven through the real frame transport.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilProtoTests;

import std;

import Aircraft;
import FlightControllerTypes;
import HalTypes;
import HilClock;
import HilFcTarget;
import HilTransport;
import LoopbackTransport;
import TestHarness;

namespace sim::test::hil {

namespace {
    using MissionState = sim::control::MissionState;

    /* Changes the wire setpoint during one control session without resetting the FC. */
    void test_dynamic_setpoint(sim::test::TestHarness& runner)
    {
        FlightCore::Sim::LoopbackTransport channel{};
        sim::hil::HilTransport transport{&channel};
        sim::hil::MonotonicClock clock{};
        const sim::hil::HostFcTargetConfig config{
            .controller{
                .hover_rpm = Aircraft{}.hover_rpm(),
                .spin_up_seconds = 0.01,
                .takeoff_transition_seconds = 0.01,
            },
            .dt = 0.01,
        };
        sim::hil::HostFcTarget target{channel, clock, config};
        FlightCore::HAL::SensorData sensor{
            .position_z_m = 10.0f,
            .altitude_baro_m = 10.0f,
            .wing_rpm_meas = static_cast<std::float32_t>(config.controller.hover_rpm),
            .sensor_valid_flags = 0x1Fu,
        };
        FlightCore::Transport::HilControlSetpoint setpoint{
            .target_z_m = 10.0f,
            .station_hold_seconds = 120.0f,
        };
        std::float32_t hover_command = 0.0f;
        for (std::uint16_t step = 0; step < 7; ++step) {
            sensor.timestamp_us = static_cast<std::uint64_t>(step) * 10000u;
            setpoint.target_z_m = step >= 5 ? 30.0f : 10.0f;
            const std::uint64_t sent = clock.nowUs();
            runner.check(transport.sendSensor(sensor, step, setpoint), "dynamic setpoint sent");
            const sim::hil::FcStepOutcome response = target.respond(step);
            runner.check(response.ok, "same FC instance accepts the updated setpoint");
            FlightCore::HAL::ActuatorCommands commands{};
            FlightCore::Transport::ActuatorDiagnostics diagnostics{};
            std::int64_t rtt = 0;
            const auto received =
                transport.receiveActuator(clock, clock.nowUs() + 50000,
                                          sim::hil::ActuatorExpectations{step, sensor.timestamp_us, sent},
                                          sim::hil::ActuatorReceiveOutputs{commands, diagnostics, rtt});
            runner.check(received == sim::hil::ReceiveResult::Ok, "dynamic control response decoded");
            if (step == 4) {
                hover_command = commands.wing_rpm_cmd;
            }
            if (step >= 5) {
                runner.check(commands.wing_rpm_cmd > hover_command + 1.0f,
                             "changing 10 m to 30 m increases thrust without restarting control");
                runner.check(commands.mode_flags == static_cast<std::uint8_t>(MissionState::STATION_KEEPING),
                             "setpoint update preserves the current control phase");
            }
        }
    }
}

void run_proto_dynamic_setpoint_tests(sim::test::TestHarness& runner)
{
    test_dynamic_setpoint(runner);
}

}