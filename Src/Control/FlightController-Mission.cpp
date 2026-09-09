/*
Filename: Src/Control/FlightController-Mission.cpp
Description: Climb, station-keeping, and mission-state dispatch logic.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightController;

import std;

import Aircraft;
import PhysicsDispersion;

namespace sim::control {

/*
Climb phase: the altitude loop drives the wing RPM towards target.z; the
transition to STATION_KEEPING resets the PIDs and the hold timer.
*/
ControlCommand FlightController::climb_command(const TargetState&   target,
                                               const AircraftState& actual,
                                               std::float64_t       dt)
{
    const std::float64_t altitude_error = target.z - actual.z;
    const std::float64_t desired_speed = std::clamp(altitude_error * std::float64_t{0.5},
                                                    -config_.climb_speed_mps,
                                                    config_.climb_speed_mps);
    const std::float64_t commanded_acceleration = config_.vertical_speed_gain * (desired_speed - actual.vz);
    const std::float64_t acceleration_ratio =
        std::max(std::float64_t{0.0}, std::float64_t{1.0} + commanded_acceleration / std::float64_t{9.81});
    const std::float64_t wing_rpm = std::clamp(config_.hover_rpm * std::sqrt(acceleration_ratio),
                                               config_.min_rpm,
                                               config_.max_rpm);
    const ServoMix servos = updateAttitudeControl(updatePositionControl(target, actual, dt), actual);
    const ControlCommand command{.wing_rpm = wing_rpm,
                                 .left_servo_angle = servos.left_deg,
                                 .right_servo_angle = servos.right_deg};

    if (std::abs(altitude_error) <= config_.altitude_tolerance_m) {
        reset_pids();
        station_hold_timer_ = 0.0;
        mission_state_ = MissionState::STATION_KEEPING;
    }
    return command;
}

/*
Station keeping phase: accumulates the hold timer while the vehicle stays inside
the target zone and completes the mission after station_hold_seconds.
*/
ControlCommand FlightController::station_keeping_step(const TargetState&   target,
                                                      const AircraftState& actual,
                                                      std::float64_t       dt)
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
Mission state machine: SPIN_UP -> TAKEOFF -> CLIMB -> STATION_KEEPING ->
COMPLETE. ABORTED and FAILED are terminal states reached only by the SIL engine.
*/
ControlCommand FlightController::update(const TargetState&   target,
                                        const AircraftState& actual,
                                        std::float64_t       dt)
{
    switch (mission_state_) {
    case MissionState::SPIN_UP:
        return spin_up_command(dt);
    case MissionState::TAKEOFF:
        return takeoff_command(actual, dt);
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
