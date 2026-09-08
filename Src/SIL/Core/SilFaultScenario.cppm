/*
Filename: Src/SIL/Core/SilFaultScenario.cppm
Description: Declarative fault taxonomy : fault domain, failure mode, target, temporality, value kind, parameters and activation window.
Exports:
    enum class FaultDomain,
    enum class FailureMode,
    enum class FaultTarget,
    enum class FaultProfile,
    enum class FaultValueKind,
    struct FaultParameters,
    struct FaultScenario,
    fault_domain_name(),
    failure_mode_name(),
    failure_mode_domain(),
    failure_mode_implemented(),
    fault_target_name(),
    fault_target_signal(),
    fault_target_physical_role(),
    fault_target_category(),
    fault_target_function(),
    effective_fault_target(),
    fault_profile_name(),
    fault_value_kind_name()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilFaultScenario;

import std;

import Telemetry;

export namespace sim::sil {

/*
Top-level problem taxonomy: the domain a fault or error belongs to. SYSTEM
covers real aircraft / embedded system failures (the only domain the FMECA
focuses on), HIL covers hardware-in-the-loop bench and protocol failures and
INFRASTRUCTURE covers test framework, filesystem, CLI and configuration
errors.
*/
enum class FaultDomain { SYSTEM, HIL, INFRASTRUCTURE };

/*
Failure modes of the aircraft / embedded system (all FaultDomain::SYSTEM). A
failure mode names the root cause only, never the detection mechanism: the
detection events produced by the FC2 supervision live in
sim::safety::DetectionEvent. CONTROL_DEADLINE_MISSED and
INVALID_NUMERICAL_STATE are documented taxonomy entries without an injection
path yet; failure_mode_implemented() reports the honest coverage and the
injector factory rejects them.
*/
enum class FailureMode {
    NONE,
    FC1_UNAVAILABLE,
    FC_COMMUNICATION_LOSS,
    COMMUNICATION_DEGRADED,
    INVALID_SENSOR_DATA,
    ACTUATOR_DEGRADED,
    CONTROL_DEADLINE_MISSED,
    INVALID_NUMERICAL_STATE
};

// Targeted component of a fault injection (canonical identifier per value).
enum class FaultTarget {
    Unspecified,
    ActuatorMainRotor,
    ActuatorLeftServo,
    ActuatorRightServo,
    SensorBarometer,
    SensorImu,
    SensorGnss,
    SensorRpmFeedback,
    ProcessorFc1,
    LinkFc1Fc2
};

// Temporality of a fault activation window (duration <= 0 means permanent).
enum class FaultProfile { Permanent, Temporary };

/*
Semantic of the numeric parameter carried by a fault event: actuator
efficiency, communication loss probability, forced altitude measurement or
noise amplitude.
*/
enum class FaultValueKind { None, Efficiency, LossProbability, Altitude, AltitudeNoise };

struct FaultParameters {
    std::float64_t       loss_probability = 0.0;
    SensorCorruptionMode corruption = SensorCorruptionMode::None;
    std::float64_t       corrupted_altitude_m = 99999.0;
    std::float64_t       efficiency = 1.0;
};

struct FaultScenario {
    std::float64_t  start_time = 0.0;
    std::float64_t  duration = 0.0;
    FailureMode     failure_mode = FailureMode::NONE;
    FaultTarget     target = FaultTarget::Unspecified;
    FaultParameters parameters{};
};

// Human-readable name of a fault domain for reports.
[[nodiscard]] std::string_view fault_domain_name(FaultDomain domain);

// Human-readable name of a failure mode for reports.
[[nodiscard]] std::string_view failure_mode_name(FailureMode mode);

/*
Domain of a failure mode: every injectable failure mode is an aircraft /
embedded system failure (FaultDomain::SYSTEM). HIL bench and infrastructure
problems are reported through their own typed errors, never as failure modes.
*/
[[nodiscard]] constexpr FaultDomain failure_mode_domain(FailureMode) noexcept
{
    return FaultDomain::SYSTEM;
}

/*
Honest implementation coverage of a failure mode: FC1 unavailability,
communication loss/degradation, invalid sensor data and actuator degradation
have an injection path; the documented timing and numerical failure modes do
not yet.
*/
[[nodiscard]] constexpr bool failure_mode_implemented(FailureMode mode) noexcept
{
    return mode != FailureMode::CONTROL_DEADLINE_MISSED && mode != FailureMode::INVALID_NUMERICAL_STATE;
}

/* Canonical identifier of a fault target for the Target field of the injection logs. */
[[nodiscard]] std::string_view fault_target_name(FaultTarget target);

// Simulated signal path disturbed on the target (empty when not applicable).
[[nodiscard]] std::string_view fault_target_signal(FaultTarget target);

// Physical role of a fault target (motor_front_left, main_rotor); empty for non-actuator targets.
[[nodiscard]] std::string_view fault_target_physical_role(FaultTarget target);

// Physical category of a fault target (ROTOR_MOTOR, CONTROL_SURFACE_SERVO); empty for non-actuator targets.
[[nodiscard]] std::string_view fault_target_category(FaultTarget target);

// Aerodynamic or propulsion function of a fault target; empty for non-actuator targets.
[[nodiscard]] std::string_view fault_target_function(FaultTarget target);

/*
Target actually disturbed by the scenario: the explicit target when the
scenario names one, otherwise the canonical component altered by the failure
mode (FC1 processor, FC1-FC2 link, altitude channel or main-rotor actuator).
*/
[[nodiscard]] FaultTarget effective_fault_target(const FaultScenario& scenario);

// Human-readable temporality profile of a fault activation window.
[[nodiscard]] std::string_view fault_profile_name(FaultProfile profile);

// Human-readable semantic of the numeric parameter of a fault event.
[[nodiscard]] std::string_view fault_value_kind_name(FaultValueKind kind);

}
