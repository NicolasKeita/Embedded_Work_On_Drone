/*
Filename: Src/SIL/Core/SilFaultScenario.cppm
Description: Declarative fault vocabulary : type, target, temporality, value kind, parameters and activation window.
Exports:
    enum class FaultType,
    enum class FaultTarget,
    enum class FaultProfile,
    enum class FaultValueKind,
    struct FaultParameters,
    struct FaultScenario,
    fault_type_name(),
    fault_target_name(),
    fault_target_signal(),
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

enum class FaultType {
    None,
    FC1Failure,
    CommunicationLoss,
    CommunicationLossRate,
    SensorFault,
    ActuatorDegradation
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
    FaultType       fault_type = FaultType::None;
    FaultTarget     target = FaultTarget::Unspecified;
    FaultParameters parameters{};
};

// Human-readable name of a fault type for reports.
[[nodiscard]] std::string_view fault_type_name(FaultType type);

/* Canonical identifier of a fault target for the Target field of the injection logs. */
[[nodiscard]] std::string_view fault_target_name(FaultTarget target);

// Simulated signal path disturbed on the target (empty when not applicable).
[[nodiscard]] std::string_view fault_target_signal(FaultTarget target);

/*
Target actually disturbed by the scenario: the explicit target when the
scenario names one, otherwise the canonical component altered by the fault
family (FC1 processor, FC1-FC2 link, altitude channel or main-rotor actuator).
*/
[[nodiscard]] FaultTarget effective_fault_target(const FaultScenario& scenario);

// Human-readable temporality profile of a fault activation window.
[[nodiscard]] std::string_view fault_profile_name(FaultProfile profile);

// Human-readable semantic of the numeric parameter of a fault event.
[[nodiscard]] std::string_view fault_value_kind_name(FaultValueKind kind);

}
