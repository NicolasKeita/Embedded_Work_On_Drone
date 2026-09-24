/*
Filename: Src/Embedded/Hil/Config/HilConfig.cppm
Description: Public interface of the HIL runner configuration : control period, mission
window, target/controller (re-used from the existing project config), deadline policy,
transport timeout and report cadence.
Exports:
    enum class DeadlinePolicy,
    struct HilConfig

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilConfig;

import std;

import Aircraft;
import FlightControllerTypes;
import Telemetry;

export namespace sim::hil {

/*
Behavior on a wall-clock deadline miss. Warn records and continues (conservative
default for validation), Fail records and forces a FAIL verdict, Abort stops the
mission loop immediately.
*/
enum class DeadlinePolicy {
    Warn,
    Fail,
    Abort
};

enum class InterfaceSelection : std::uint8_t {
    ExplicitLoopback,
    ExplicitSerial,
    AutoTrustedStm32,
    AutoFallbackNotFound,
    AutoFallbackAmbiguous,
    AutoFallbackUnavailable,
    RejectedUntrustedDevice
};

struct HilConfig {
    /* Resolved byte channel: "loopback" or a stable Linux serial-device path. */
    std::string        interface_name{"loopback"};
    std::string        fc1_stlink_serial{};
    std::string        fc2_stlink_serial{};
    InterfaceSelection interface_selection = InterfaceSelection::ExplicitLoopback;

    /*
    Control period (s). Mirrors the project HIL ControlTask rate of 100 Hz / 10 ms
    (hil_protocol.md 1.3, SilConfig::dt). The runner derives the step count from
    duration_s / dt_s, so it is never hard-coded.
    */
    std::float64_t dt_s = 0.01;

    /* Mission window (s). Mirrors SilConfig::duration_s. */
    std::float64_t duration_s = 30.0;

    /*
    Mission target. Default altitude (10 m) matches the validated SIL nominal
    scenario (SilConfig::target.z); the controller gains come from the same
    ControllerConfig the SIL and the embedded FC1 share.
    */
    sim::control::TargetState      target{.z = 10.0};
    sim::control::ControllerConfig controller{.hover_rpm = Aircraft{}.hover_rpm()};

    /* Safety/health parameters reused from the SIL baseline (SilConfig). */
    std::float64_t                   heartbeat_timeout_s = 0.10;
    std::float64_t                   actuator_mismatch_rpm = 60.0;
    std::float64_t                   actuator_mismatch_hold_s = 0.50;
    std::float64_t                   thrust_compensation_margin = 1.7;
    sim::sil::SensorValidationLimits sensor_limits{};

    /* Horizontal wind disturbance, active on [start, end), with optional periodic gusts. */
    std::float64_t wind_x_mps = 0.0;
    std::float64_t wind_y_mps = 0.0;
    std::float64_t wind_start_s = 8.0;
    std::float64_t wind_end_s = 24.0;
    std::float64_t wind_gust_period_s = 0.0;

    /* Determinism identifiers. */
    std::uint64_t    seed = 42;
    std::string_view scenario_id = "NOMINAL-001";

    /* Telemetry/report cadence: structured internal rate + 1 Hz human-readable. */
    std::float64_t telemetry_rate_hz = 20.0;
    std::float64_t report_period_s = 1.0;

    /*
    Sensor measurement noise standard deviation (m for position/altitude,
    m/s for velocity, rad for attitude). 0.0 yields perfect deterministic sensors
    (matches the validated SIL nominal behaviour) so the nominal mission is
    bit-reproducible; a non-zero value makes the truth/sensor distinction visible.
    */
    std::float64_t sensor_noise_stddev = 0.0;

    DeadlinePolicy deadline_policy = DeadlinePolicy::Warn;

    /*
    Wall-clock budget (us) to wait for one ActuatorPacket; defaults to the control
    period so a single missed response is detectable before the next deadline.
    */
    std::uint64_t transport_timeout_us = 10000;
};

/* Returns the instantaneous deterministic wind multiplier at simulation time. */
[[nodiscard]] std::float64_t wind_factor(const HilConfig& config, std::float64_t time_s) noexcept;

/* Human-readable name of a deadline policy. */
[[nodiscard]] std::string_view deadline_policy_name(DeadlinePolicy policy) noexcept;

/* Number of control steps implied by the configured window (duration / dt). */
[[nodiscard]] std::uint64_t hil_step_count(const HilConfig& config) noexcept;

}
