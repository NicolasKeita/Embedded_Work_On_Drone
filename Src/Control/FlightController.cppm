/*
Filename: Src/Control/FlightController.cppm
Description: Public interface of the autonomous flight controller : mission
state machine and cascaded position, altitude and attitude loops.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FlightController;

import std;

import Aircraft;

export namespace sim::control {

struct TargetState {
    std::float64_t x = 0.0;
    std::float64_t y = 0.0;
    std::float64_t z = 0.0;
};

enum class MissionState {
    TAKEOFF,
    CLIMB,
    STATION_KEEPING,
    COMPLETE,
    ABORTED,
    FAILED
};

struct ControllerConfig {
    std::float64_t hover_rpm{0.0};
    std::float64_t kp_altitude{5.0};
    std::float64_t ki_altitude{0.05};
    std::float64_t kd_altitude{24.0};
    std::float64_t max_integral_rpm{60.0};
    std::float64_t kp_position{1.0};
    std::float64_t kd_position{5.1};
    std::float64_t kp_attitude{0.8};
    std::float64_t max_tilt_deg{15.0};
    std::float64_t min_rpm{300.0};
    std::float64_t max_rpm{12000.0};
    std::float64_t takeoff_rpm_factor{1.3};
    std::float64_t takeoff_altitude_m{2.0};
    std::float64_t altitude_tolerance_m{0.5};
    std::float64_t position_tolerance_m{1.0};
    std::float64_t station_hold_seconds{5.0};
};

// Tilt setpoints produced by the position loop (outer loop).
struct TiltTargets {
    std::float64_t pitch_deg = 0.0;
    std::float64_t roll_deg = 0.0;
};

// Servo mix produced by the attitude loop (inner loop).
struct ServoMix {
    std::float64_t left_deg = 0.0;
    std::float64_t right_deg = 0.0;
};

class FlightController {
public:
    explicit FlightController(ControllerConfig config);

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

    ControllerConfig config_;
    MissionState     mission_state_{MissionState::TAKEOFF};
    AxisPid          altitude_pid_{};
    AxisPid          x_position_pid_{};
    AxisPid          y_position_pid_{};
    std::float64_t   station_hold_timer_{0.0};
};

// Human-readable name of a mission state for logging.
[[nodiscard]] std::string_view mission_state_name(MissionState state);

}
