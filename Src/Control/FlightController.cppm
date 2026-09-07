/*
Filename: Src/Control/FlightController.cppm
Description: Public interface of the autonomous flight controller : cascaded loops and mission state
(types come from FlightControllerTypes).
Exports:
    class FlightController

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FlightController;

import std;

import Aircraft;
import PhysicsDispersion;

export import FlightControllerTypes;

export namespace sim::control {

class FlightController {
public:
    explicit FlightController(ControllerConfig config, const sim::PhysicsDispersion& dispersion = {});

    [[nodiscard]] MissionState state() const;

    /*
    Advances the mission state machine by one time step dt and returns the
    actuator command (wing RPM + left/right servo angles) computed by the
    cascaded loops from the target setpoint and the measured state.
    */
    ControlCommand update(const TargetState& target, const AircraftState& actual, std::float64_t dt);

private:
    struct AxisPid {
        std::float64_t kp = 0.0;
        std::float64_t ki = 0.0;
        std::float64_t kd = 0.0;
        std::float64_t integral = 0.0;
        std::float64_t integral_limit = 0.0;
        std::float64_t integral_error_band = 0.0;
        std::float64_t previous_error = 0.0;
        bool           primed = false;
    };

    void enter_climb();
    void reset_pids();
    static std::float64_t AxisPidStep(AxisPid& pid, std::float64_t error, std::float64_t dt);
    std::float64_t updateAltitudeControl(std::float64_t target_z, std::float64_t actual_z, std::float64_t dt);
    TiltTargets updatePositionControl(const TargetState& t, const AircraftState& a, std::float64_t dt);
    ServoMix updateAttitudeControl(TiltTargets tilt, const AircraftState& s) const;
    ControlCommand station_keeping_command(const TargetState& t, const AircraftState& a,
                                           std::float64_t dt);
    ControlCommand takeoff_command(const AircraftState& actual);
    ControlCommand climb_command(const TargetState& target, const AircraftState& actual, std::float64_t dt);
    ControlCommand station_keeping_step(const TargetState& target, const AircraftState& actual,
                                        std::float64_t dt);
    [[nodiscard]] bool inside_target_zone(const TargetState& t, const AircraftState& a) const;

    ControllerConfig       config_;
    MissionState           mission_state_{MissionState::TAKEOFF};
    AxisPid                altitude_pid_{};
    AxisPid                x_position_pid_{};
    AxisPid                y_position_pid_{};
    std::float64_t         station_hold_timer_{0.0};
    sim::PhysicsDispersion dispersion_{};
};

}
