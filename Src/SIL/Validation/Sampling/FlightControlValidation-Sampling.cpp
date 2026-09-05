/*
Filename: Src/SIL/Validation/Sampling/FlightControlValidation-Sampling.cpp
Description: RNG-driven sampling of the fault family and fault-specific parameters for a Scenario.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightControlValidation;

import std;

import SilTypes;
import Telemetry;

namespace sim::sil::validation {

/*
Samples the fault family and its activation window. The fault type is drawn
first so that the rest of the scenario can branch on it (a sensor fault needs
a corruption mode, a comms fault needs a loss probability, ...).
*/
void sample_fault_window(Scenario& scenario, std::mt19937_64& generator)
{
    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);
    std::uniform_real_distribution<std::float64_t> time_window(2.0, 12.0);
    std::uniform_real_distribution<std::float64_t> duration_window(1.0, 8.0);
    const std::uint64_t                            fault_roll = generator() % 6;

    scenario.fault_type = static_cast<FaultType>(fault_roll);
    scenario.fault_start_time_s = time_window(generator);
    scenario.fault_duration_s = duration_window(generator);
    scenario.fault_profile = (unit(generator) < 0.5) ? FaultProfile::Permanent : FaultProfile::Temporary;
    if (scenario.fault_duration_s <= 0.0) {
        scenario.fault_profile = FaultProfile::Permanent;
    }
    const std::uint64_t target_roll = generator() % 10;
    scenario.fault_target = static_cast<FaultTarget>(target_roll);
}

/*
Branches on the sampled fault type to set the fault-specific parameters:
processor target, link cutoff, loss rate, sensor corruption mode or actuator
efficiency. Each branch consumes exactly the RNG draws it needs.
*/
void sample_fault_parameters(Scenario& scenario, std::mt19937_64& generator)
{
    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);

    switch (scenario.fault_type) {
    case FaultType::None:
        break;
    case FaultType::FC1Failure:
        scenario.fault_target = FaultTarget::ProcessorFc1;
        break;
    case FaultType::CommunicationLoss:
        scenario.fault_target = FaultTarget::LinkFc1Fc2;
        scenario.fault_loss_probability = 1.0;
        scenario.comms_link_up = false;
        break;
    case FaultType::CommunicationLossRate:
        scenario.fault_target = FaultTarget::LinkFc1Fc2;
        scenario.fault_loss_probability = unit(generator) * 0.8;
        scenario.comms_loss_probability = scenario.fault_loss_probability;
        break;
    case FaultType::SensorFault: {
        const std::uint64_t corruption_roll = generator() % 3;
        scenario.sensor_corruption = static_cast<SensorCorruptionMode>(corruption_roll + 1);
        scenario.corrupted_altitude_m = (unit(generator) - 0.5) * 1000.0;
        scenario.fault_target = FaultTarget::SensorBarometer;
        break;
    }
    case FaultType::ActuatorDegradation:
        scenario.actuator_efficiency = 0.4 + unit(generator) * 0.5;
        scenario.fault_target = FaultTarget::ActuatorMainRotor;
        break;
    }
}

}
