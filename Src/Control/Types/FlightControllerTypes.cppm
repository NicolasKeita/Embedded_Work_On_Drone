/*
Filename: Src/Control/Types/FlightControllerTypes.cppm
Description: Data types and state naming of the flight controller interface.
Exports:
    enum class MissionState,
    struct TargetState,
    struct ControllerConfig,
    struct TiltTargets,
    struct ServoMix,
    mission_state_name()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FlightControllerTypes;

import std;

export namespace sim::control {

struct TargetState {
    std::float64_t x = 0.0;
    std::float64_t y = 0.0;
    std::float64_t z = 0.0;
};

enum class MissionState {
    SPIN_UP,
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
    std::float64_t altitude_tolerance_m{0.5};
    std::float64_t position_tolerance_m{1.0};
    std::float64_t station_hold_seconds{5.0};
    std::float64_t spin_up_seconds{3.0};
    std::float64_t takeoff_transition_seconds{12.0};
    std::float64_t climb_speed_mps{0.617};
    std::float64_t vertical_speed_gain{1.5};
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

// Human-readable name of a mission state for logging.
[[nodiscard]] std::string_view mission_state_name(MissionState state);

}
