/*
Filename: Src/Control/FlightController-Takeoff.cpp
Description: Progressive rotor spin-up and smooth vertical-speed takeoff control.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightController;

import std;

import Aircraft;

namespace sim::control {

/*
Progressively spins the wing from rest to hover RPM while ground contact keeps
the aircraft stationary, then enters the takeoff phase.
*/
ControlCommand FlightController::spin_up_command(std::float64_t dt)
{
    phase_elapsed_seconds_ = std::min(phase_elapsed_seconds_ + dt, config_.spin_up_seconds);
    const std::float64_t progress = config_.spin_up_seconds > 0.0
        ? phase_elapsed_seconds_ / config_.spin_up_seconds
        : 1.0;
    const std::float64_t smooth_progress = progress * progress * (3.0 - 2.0 * progress);
    const std::float64_t wing_rpm = smooth_progress * config_.hover_rpm;

    if (progress >= 1.0) {
        phase_elapsed_seconds_ = 0.0;
        mission_state_ = MissionState::TAKEOFF;
    }
    return ControlCommand{.wing_rpm = wing_rpm};
}

/*
Tracks a smoothstep vertical-speed profile up to the Heliblade-derived climb
speed. The acceleration feed-forward avoids an instantaneous velocity step.
*/
ControlCommand FlightController::takeoff_command(const AircraftState& actual, std::float64_t dt)
{
    phase_elapsed_seconds_ = std::min(phase_elapsed_seconds_ + dt, config_.takeoff_transition_seconds);
    const std::float64_t progress = config_.takeoff_transition_seconds > 0.0
        ? phase_elapsed_seconds_ / config_.takeoff_transition_seconds
        : 1.0;
    const std::float64_t desired_speed = config_.climb_speed_mps * progress * progress * (3.0 - 2.0 * progress);
    const std::float64_t desired_acceleration = config_.takeoff_transition_seconds > 0.0
        ? config_.climb_speed_mps * 6.0 * progress * (1.0 - progress) / config_.takeoff_transition_seconds
        : 0.0;
    const std::float64_t commanded_acceleration =
        desired_acceleration + config_.vertical_speed_gain * (desired_speed - actual.vz);
    const std::float64_t acceleration_ratio =
        std::max(std::float64_t{0.0}, std::float64_t{1.0} + commanded_acceleration / std::float64_t{9.81});
    const std::float64_t wing_rpm = std::clamp(config_.hover_rpm * std::sqrt(acceleration_ratio),
                                               config_.min_rpm,
                                               config_.max_rpm);

    if (progress >= 1.0) {
        phase_elapsed_seconds_ = 0.0;
        enter_climb();
    }
    return ControlCommand{.wing_rpm = wing_rpm};
}

}
