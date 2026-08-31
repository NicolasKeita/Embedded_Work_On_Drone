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
    SensorTelemetry telemetry;

    telemetry.x = state.x;
    telemetry.y = state.y;
    telemetry.z = state.z;
    telemetry.vx = state.vx;
    telemetry.vy = state.vy;
    telemetry.vz = state.vz;
    telemetry.pitch = state.pitch;
    telemetry.roll = state.roll;
    telemetry.actual_rpm = state.actual_rpm;
    telemetry.actual_left_servo = state.actual_left_servo;
    telemetry.actual_right_servo = state.actual_right_servo;
    return telemetry;
}

/*
Rebuilds the flight-controller input view from the sensor chain. Angular rates
are not consumed by the controller and are left at rest.
*/
AircraftState to_aircraft_state(const SensorTelemetry& telemetry)
{
    AircraftState state;

    state.x = telemetry.x;
    state.y = telemetry.y;
    state.z = telemetry.z;
    state.vx = telemetry.vx;
    state.vy = telemetry.vy;
    state.vz = telemetry.vz;
    state.pitch = telemetry.pitch;
    state.roll = telemetry.roll;
    state.actual_rpm = telemetry.actual_rpm;
    state.actual_left_servo = telemetry.actual_left_servo;
    state.actual_right_servo = telemetry.actual_right_servo;
    return state;
}

}
