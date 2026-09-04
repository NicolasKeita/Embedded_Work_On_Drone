/*
Filename: Src/SIL/Core/Telemetry/Telemetry-Chain.cpp
Description: Sensor chain building, view conversion, validation and corruption.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Telemetry;

import std;

import Aircraft;

namespace sim::sil {

bool SensorValidity::all_valid() const noexcept
{
    return altitude_valid && position_valid;
}

/*
Copies the physics ground truth into the simulated sensor chain: without
injected corruption the sensors are ideal and both views coincide.
*/
SensorTelemetry make_telemetry(const AircraftState& state)
{
    SensorTelemetry telemetry{.x = state.x,
                              .y = state.y,
                              .z = state.z,
                              .vx = state.vx,
                              .vy = state.vy,
                              .vz = state.vz,
                              .pitch = state.pitch,
                              .roll = state.roll,
                              .actual_rpm = state.actual_rpm,
                              .actual_left_servo = state.actual_left_servo,
                              .actual_right_servo = state.actual_right_servo};

    return telemetry;
}

/*
Rebuilds the flight-controller input view from the sensor chain. Angular rates
are not consumed by the controller and are left at rest.
*/
AircraftState to_aircraft_state(const SensorTelemetry& telemetry)
{
    AircraftState state{.x = telemetry.x,
                        .y = telemetry.y,
                        .z = telemetry.z,
                        .vx = telemetry.vx,
                        .vy = telemetry.vy,
                        .vz = telemetry.vz,
                        .pitch = telemetry.pitch,
                        .roll = telemetry.roll,
                        .actual_rpm = telemetry.actual_rpm,
                        .actual_left_servo = telemetry.actual_left_servo,
                        .actual_right_servo = telemetry.actual_right_servo};

    return state;
}

}
