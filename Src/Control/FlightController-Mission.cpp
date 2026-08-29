/*
Filename: Src/Control/FlightController-Mission.cpp
Description: Mission state machine of the autonomous flight controller (TAKEOFF to COMPLETE).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightController;

import std;

import Aircraft;

namespace sim::control {

namespace
{
    double Clamp(double value, double minValue, double maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

/*
Takeoff phase: arms the wing at a fixed RPM (takeoff_rpm_factor x hover_rpm) and
switches to CLIMB as soon as takeoff_altitude_m is reached.
*/
ControlCommand FlightController::takeoff_command(const AircraftState& actual)
{
    ControlCommand cmd;
    cmd.wing_rpm = Clamp(config_.takeoff_rpm_factor * config_.hover_rpm, config_.min_rpm,
                         config_.max_rpm);

    if (actual.z >= config_.takeoff_altitude_m) {
        enter_climb();
    }
    return cmd;
}

/*
Climb phase: the altitude loop drives the wing RPM towards target.z; the
transition to STATION_KEEPING resets the PIDs and the hold timer.
*/
ControlCommand FlightController::climb_command(const TargetState& target, const AircraftState& actual, double dt)
{
    ControlCommand cmd;
    cmd.wing_rpm = updateAltitudeControl(target.z, actual.z, dt);

    if (std::abs(target.z - actual.z) <= config_.altitude_tolerance_m) {
        reset_pids();
        station_hold_timer_ = 0.0;
        mission_state_ = MissionState::STATION_KEEPING;
    }
    return cmd;
}

/*
Station keeping phase: accumulates the hold timer while the vehicle stays inside
the target zone and completes the mission after station_hold_seconds.
*/
ControlCommand FlightController::station_keeping_step(const TargetState&   target,
                                                      const AircraftState& actual,
                                                      double               dt)
{
    ControlCommand cmd = station_keeping_command(target, actual, dt);

    if (inside_target_zone(target, actual)) {
        station_hold_timer_ += dt;
        if (station_hold_timer_ >= config_.station_hold_seconds) {
            mission_state_ = MissionState::COMPLETE;
        }
    }
    else {
        station_hold_timer_ = 0.0;
    }
    return cmd;
}

/*
Mission state machine: TAKEOFF (fixed RPM) then CLIMB (altitude loop towards
target.z) then STATION_KEEPING (simultaneous X/Y/Z control, COMPLETE after
station_hold_seconds consecutively inside the target zone) then COMPLETE
(continuous station keeping to maintain the position).
*/
ControlCommand FlightController::update(const TargetState&   target,
                                        const AircraftState& actual,
                                        double               dt)
{
    switch (mission_state_) {
    case MissionState::TAKEOFF:
        return takeoff_command(actual);
    case MissionState::CLIMB:
        return climb_command(target, actual, dt);
    case MissionState::STATION_KEEPING:
        return station_keeping_step(target, actual, dt);
    case MissionState::COMPLETE:
        return station_keeping_command(target, actual, dt);
    }

    return ControlCommand{};
}

std::string_view mission_state_name(MissionState state)
{
    switch (state) {
    case MissionState::TAKEOFF:
        return "TAKEOFF";
    case MissionState::CLIMB:
        return "CLIMB";
    case MissionState::STATION_KEEPING:
        return "STATION_KEEPING";
    case MissionState::COMPLETE:
        return "COMPLETE";
    }
    return "UNKNOWN";
}

}
