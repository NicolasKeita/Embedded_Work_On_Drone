/*
Filename: Tests/Mission/MissionRunner.cppm
Description: Autonomous mission run loop with tracking metrics and reporting helpers.
Exports: TrackingAxis, MissionMetrics, MissionRunRequest, MissionRunTrace,
component_value(), print_metrics_report(), contains_mission_sequence(), run_mission().

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module MissionRunner;

import std;

import Aircraft;
import FlightController;
import PhysicsDispersion;
import SilRuntimeConfig;

using sim::control::FlightController;

export namespace sim::test {

enum class TrackingAxis { x_axis, y_axis, z_axis };

// Fixed capacity of the visited-state trace (no dynamic allocation).
inline constexpr std::size_t kMaxVisitedStates = 16;

struct MissionMetrics {
    std::float64_t initial_gap = 0.0;
    std::float64_t time_within_tolerance = -1.0;
    std::float64_t overshoot_units = 0.0;
    std::float64_t steady_state_error = 0.0;
    std::float64_t final_error = 0.0;
    std::float64_t max_acceleration = 0.0;

    [[nodiscard]] std::float64_t overshoot_percent() const noexcept;
};

// Component of the measured state associated with the requested tracking axis.
[[nodiscard]] std::float64_t component_value(const AircraftState& state, TrackingAxis axis);
// Component of the target setpoint for the requested tracking axis.
[[nodiscard]] std::float64_t component_value(const sim::control::TargetState& target, TrackingAxis axis);
// Prints the tracking indicators of one mission run (tolerance, overshoot, errors).
void print_metrics_report(std::string_view label, const MissionMetrics& metrics);
// Checks that the visited states contain the expected SPIN_UP..COMPLETE mission flow.
bool contains_mission_sequence(std::span<const sim::control::MissionState> visited);

struct MissionRunRequest {
    sim::control::TargetState target;
    std::float64_t            duration = 0.0;
    TrackingAxis              axis = TrackingAxis::z_axis;
    std::float64_t            tolerance = 0.0;
    bool                      stop_on_zone = false;
    bool                      verbose = true;
    std::float64_t            dt_s = sim::host::sil_runtime_options().runner.dt;
    std::float64_t            report_period_s = sim::host::sil_runtime_options().report_period_s;
    std::float64_t            wind_x_mps = 0.0;
    std::float64_t            wind_y_mps = 0.0;
    std::float64_t            wind_start_s = 0.0;
    std::float64_t            wind_end_s = 0.0;
    std::float64_t            wind_gust_period_s = 0.0;
};

struct MissionViewerSample {
    std::float64_t             time = 0.0;
    AircraftState              aircraft{};
    sim::control::TargetState  target{};
    sim::control::MissionState mission = sim::control::MissionState::SPIN_UP;
    std::float64_t             wind_x_mps = 0.0;
    std::float64_t             wind_y_mps = 0.0;
    std::float64_t             dt_s = 0.01;
};

using MissionViewerCallback = void (*)(const MissionViewerSample&, void*);

struct MissionViewerObserver {
    MissionViewerCallback callback = nullptr;
    void*                 context = nullptr;
};

/* Registers the process-local observer used to visualize one accelerated mission. */
void set_mission_viewer_observer(MissionViewerObserver observer) noexcept;

struct MissionRunTrace {
    std::array<sim::control::MissionState, kMaxVisitedStates> visited_states{};
    std::size_t                                               visited_count = 0;
    MissionMetrics                                            metrics;

    void record(sim::control::MissionState state);
};

// Advances one autonomous mission (controller, physics, logging, metrics, tracing).
MissionRunTrace run_mission(FlightController& ctrl, Aircraft& craft,
                            const MissionRunRequest& run,
                            const sim::PhysicsDispersion& dispersion = {});

// Internal loop pieces shared by the MissionRunner-*.cpp translation units.
/* Bundles the controlled plant (FlightController + Aircraft) shared by every mission loop helper. */
struct MissionDynamics {
    FlightController& ctrl;
    Aircraft&         craft;
};
struct StepMetrics {
    std::float64_t direction = 1.0;
    std::float64_t steady_start = 0.0;
    std::float64_t error_sum = 0.0;
    std::float64_t samples = 0.0;
    std::float64_t max_acceleration = 0.0;
    AircraftState  previous_state{};

    void update(MissionMetrics& metrics, std::float64_t time, std::float64_t error, std::float64_t tolerance);
    void track_acceleration(std::float64_t dt, const AircraftState& current);
    void close(MissionMetrics& metrics, std::float64_t target_value, TrackingAxis axis,
               const AircraftState& final_state);
    [[nodiscard]] std::float64_t steady_state_error(std::float64_t fallback) const;
};

std::float64_t steady_window_start(std::float64_t duration);
void advance_step(MissionDynamics dynamics, StepMetrics& step, MissionRunTrace& trace,
                  const MissionRunRequest& run, std::float64_t& time);
void publish_viewer_sample(MissionDynamics dynamics, const MissionRunRequest& run, std::float64_t time,
                           std::float64_t& next_viewer_time, std::float64_t viewer_period);
void print_state_row(const Aircraft& aircraft, std::float64_t timeSeconds);
void log_state_transition(MissionRunTrace& trace, sim::control::MissionState& previous,
                          const sim::control::MissionState current, std::float64_t time, bool verbose);
bool zone_reached(const MissionRunRequest& run, sim::control::MissionState state,
                  const AircraftState& s);
void run_control_loop(MissionDynamics dynamics, const MissionRunRequest& run, MissionRunTrace& trace,
                      std::float64_t direction, std::float64_t target_value);
}
