/*
Filename: Src/SIL/Reporting/Report/SilReporting-Streams.cpp
Description: Raw sensor telemetry and ground-truth CSV stream writers.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilTelemetry;
import SilTypes;

namespace sim::sil {

/*
Writes the raw structured sensor telemetry of every scenario record: one row
per sample with the scenario name, the sensor-path flight data, the actuator
commands and the control context (targets and state enum values).
*/
void write_telemetry_csv(std::ostream& out, std::span<const ScenarioRecord> records)
{
    out << "scenario;timestamp;x;y;z;vx;vy;vz;pitch_rad;roll_rad;commanded_rpm;actual_rpm;"
           "left_servo_deg;right_servo_deg;target_x;target_y;target_z;mission_state;"
           "mission_state_name;safety_state;safety_state_name\n";
    for (const ScenarioRecord& record : records) {
        for (const TelemetrySample& sample : record.telemetry) {
            write_csv_escaped(out, record.name);
            out << ';';
            write_seconds(out, sample.time);
            out << ';' << sample.x << ';' << sample.y << ';' << sample.altitude_m << ';' << sample.vx
                << ';' << sample.vy << ';' << sample.vz << ';' << sample.pitch_rad << ';' << sample.roll_rad
                << ';' << sample.commanded_rpm << ';' << sample.actual_rpm << ';' << sample.left_servo_deg
                << ';' << sample.right_servo_deg << ';' << sample.target_x << ';' << sample.target_y << ';'
                << sample.target_z << ';' << sample.mission_state << ';'
                << sim::control::mission_state_name(static_cast<sim::control::MissionState>(sample.mission_state))
                << ';' << sample.safety_state << ';'
                << sim::safety::safety_mode_name(static_cast<sim::safety::SafetyMode>(sample.safety_state))
                << '\n';
        }
    }
}

/*
Writes the raw physics ground truth of every scenario record into a separate
stream so that truth and sensor observations are never conflated.
*/
void write_truth_csv(std::ostream& out, std::span<const ScenarioRecord> records)
{
    out << "scenario;timestamp;x;y;z;vx;vy;vz;pitch_rad;roll_rad;actual_rpm;actual_left_servo;"
           "actual_right_servo\n";
    for (const ScenarioRecord& record : records) {
        for (const TrueStateSample& sample : record.ground_truth) {
            write_csv_escaped(out, record.name);
            out << ';';
            write_seconds(out, sample.time);
            out << ';' << sample.x << ';' << sample.y << ';' << sample.z << ';' << sample.vx << ';'
                << sample.vy << ';' << sample.vz << ';' << sample.pitch << ';' << sample.roll << ';'
                << sample.actual_rpm << ';' << sample.actual_left_servo << ';'
                << sample.actual_right_servo << '\n';
        }
    }
}

}