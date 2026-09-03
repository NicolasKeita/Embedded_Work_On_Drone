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

/*
Takeoff phase: arms the wing at a fixed RPM (takeoff_rpm_factor x hover_rpm) and
switches to CLIMB as soon as takeoff_altitude_m is reached.
*/
ControlCommand FlightController::takeoff_command(const AircraftState& actual)
{
    ControlCommand cmd;

    cmd.wing_rpm = std::clamp(config_.takeoff_rpm_factor * config_.hover_rpm, config_.min_rpm, config_.max_rpm);

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
Mission state machine: TAKEOFF -> CLIMB -> STATION_KEEPING -> COMPLETE. ABORTED
and FAILED are terminal states reached only by the SIL engine (safety abort,
mission window exhausted): the controller answers them with a zeroed command.
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
    case MissionState::ABORTED:
    case MissionState::FAILED:
        return ControlCommand{};
    }

    return ControlCommand{};
}

}
