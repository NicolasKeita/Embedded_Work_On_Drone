/*
Filename: Src/SIL/Core/SilFaultScenario.cppm
Description: Declarative fault scenario types of the SIL engine: fault type, parameters and timed activation window.
Exports:
    enum class FaultType,
    struct FaultParameters,
    struct FaultScenario,
    fault_type_name()

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
    FaultParameters parameters{};
};

// Human-readable name of a fault type for reports.
[[nodiscard]] std::string_view fault_type_name(FaultType type);

}
