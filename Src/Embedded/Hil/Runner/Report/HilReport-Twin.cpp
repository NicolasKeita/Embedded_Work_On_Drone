/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport-Twin.cpp
Description: TwinSnapshot JSON serialization for the Digital Twin visualization stream.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

import FlightControllerTypes;
import HealthMonitor;
import HilEvents;
import HilRunnerContext;
import HilTelemetry;
import SafetyManager;
import SilFaultScenario;

namespace sim::hil {

namespace {
    /* Returns the logical FC1 component affected by the active injected failure. */
    std::string_view affected_component(const HilRunContext& ctx)
    {
        using sim::sil::FailureMode;

        if (!ctx.fault_active) {
            return {};
        }
        if (ctx.last_failure_mode == FailureMode::INVALID_SENSOR_DATA) {
            return "sensors";
        }
        if (ctx.last_failure_mode == FailureMode::ACTUATOR_DEGRADED) {
            return "actuators";
        }
        if (ctx.last_failure_mode == FailureMode::FC1_UNAVAILABLE) {
            return "mcu";
        }
        if (ctx.last_failure_mode == FailureMode::FC_COMMUNICATION_LOSS
            || ctx.last_failure_mode == FailureMode::COMMUNICATION_DEGRADED) {
            return "transport";
        }
        return {};
    }

    /* Writes the aircraft, target and actuator state of one TwinSnapshot. */
    void write_aircraft_state(std::ostream& out, const HilSensorSample& sample, std::float64_t airspeed)
    {
        out << ",\"aircraft\":{\"x_m\":" << sample.x << ",\"y_m\":" << sample.y
            << ",\"z_m\":" << sample.z << ",\"altitude_m\":" << sample.z
            << ",\"pitch_rad\":" << sample.pitch_rad << ",\"roll_rad\":" << sample.roll_rad
            << ",\"airspeed_ms\":" << airspeed << '}';
        out << ",\"target\":{\"x_m\":" << sample.target_x << ",\"y_m\":" << sample.target_y
            << ",\"altitude_m\":" << sample.target_z << '}';
        out << ",\"actuators\":{\"rotor_rpm\":" << sample.measured_rpm
            << ",\"left_servo_deg\":" << sample.left_servo_deg
            << ",\"right_servo_deg\":" << sample.right_servo_deg << '}';
    }
}

/* Writes one JSON string with the escaping required by the visualization stream. */
void write_json_string(std::ostream& out, std::string_view value)
{
    out << '"';
    for (const char character : value) {
        if (character == '"' || character == '\\') {
            out << '\\';
        }
        out << character;
    }
    out << '"';
}

/* Serializes the newest structured HIL sample as a browser TwinSnapshot line. */
void write_twin_snapshot(std::ostream& out, const HilRunContext& ctx, const HilSensorSample& sample)
{
    const std::string_view affected = affected_component(ctx);
    const bool             failed = ctx.safety.mode() == sim::safety::SafetyMode::SAFE_MODE;
    const std::float64_t   airspeed = std::hypot(sample.vx, sample.vy, sample.vz);

    out << std::setprecision(8) << "{\"source\":\"HIL\",\"time_s\":" << sample.time_s;
    write_aircraft_state(out, sample, airspeed);
    out << ",\"mission\":";
    write_json_string(out, sim::control::mission_state_name(
                               static_cast<sim::control::MissionState>(sample.mission_state)));
    out << ",\"health\":";
    write_json_string(out, sim::safety::health_state_name(ctx.result.final_health));
    out << ",\"safety_mode\":";
    write_json_string(out, sim::safety::safety_mode_name(ctx.safety.mode()));
    out << ",\"active_fault\":";
    if (ctx.fault_active) {
        write_json_string(out, sim::sil::failure_mode_name(ctx.last_failure_mode));
    }
    else {
        out << "null";
    }
    out << ",\"fc1\":{\"status\":";
    write_json_string(out, ctx.fc1_was_alive ? "ONLINE" : "OFFLINE");
    out << ",\"components\":";
    write_components(out, affected, failed);
    out << "},\"fc2\":{\"status\":\"ONLINE\",\"components\":";
    write_components(out, {}, false);
    out << "},\"hil\":{\"loop_hz\":" << (1.0 / ctx.config.dt_s)
        << ",\"deadline_misses\":" << ctx.timing.deadline_misses << "},\"events\":";
    write_recent_events(out, ctx.trace.events());
    out << '}';
}

}
