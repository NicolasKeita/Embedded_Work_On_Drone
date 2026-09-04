/*
Filename: Src/SIL/Validation/FlightControlValidation-Sampling.cpp
Description: RNG-driven sampling helpers splitting a Scenario into fault, environment and initial-condition groups.

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

    const std::uint64_t fault_roll = generator() % 6;
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

/*
Samples the environment, sensor and communication disturbances that surround
the fault. Wind, pressure and temperature use normal distributions; sensor
noise and dropout rate use uniform and half-normal draws.
*/
void sample_environment(Scenario& scenario, std::mt19937_64& generator)
{
    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);
    std::normal_distribution<std::float64_t> wind_dist(0.0, 2.5);
    std::normal_distribution<std::float64_t> pressure_dist(101.325, 1.5);
    std::normal_distribution<std::float64_t> temperature_dist(288.15, 5.0);
    std::normal_distribution<std::float64_t> noise_altitude(0.0, 0.3);
    std::normal_distribution<std::float64_t> noise_position(0.0, 0.5);

    scenario.wind_x_mps = wind_dist(generator);
    scenario.wind_y_mps = wind_dist(generator);
    scenario.ambient_pressure_kpa = pressure_dist(generator);
    scenario.ambient_temperature_k = temperature_dist(generator);
    scenario.sensor_noise_altitude_m = std::abs(noise_altitude(generator));
    scenario.sensor_noise_position_m = std::abs(noise_position(generator));
    scenario.sensor_dropout_rate = unit(generator) * 0.05;
    scenario.comms_transport_latency_s = 0.001 + unit(generator) * 0.01;
}

/*
Samples the initial conditions and mission duration of the run. The initial
altitude is always zero (ground start); the horizontal offset, target altitude
and duration are uniformly distributed around their nominal values.
*/
void sample_initial_conditions(Scenario& scenario, std::mt19937_64& generator)
{
    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);

    scenario.initial_altitude_m = 0.0;
    scenario.initial_x_m = (unit(generator) - 0.5) * 4.0;
    scenario.initial_y_m = (unit(generator) - 0.5) * 4.0;
    scenario.target_altitude_m = 8.0 + unit(generator) * 6.0;
    scenario.simulation_duration_s = 25.0 + unit(generator) * 10.0;
    scenario.time_step_s = 0.01;
}

}
