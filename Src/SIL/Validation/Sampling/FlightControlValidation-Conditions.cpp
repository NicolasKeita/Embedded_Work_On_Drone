/*
Filename: Src/SIL/Validation/Sampling/FlightControlValidation-Conditions.cpp
Description: RNG-driven sampling of ambient/environment disturbances and initial conditions for a Scenario.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightControlValidation;

import std;

import SilTypes;
import Telemetry;

namespace sim::sil::validation {

/*
Samples the environment, sensor and communication disturbances that surround
the fault. Wind, pressure and temperature use normal distributions; sensor
noise and dropout rate use uniform and half-normal draws.
*/
void sample_environment(Scenario& scenario, std::mt19937_64& generator)
{
    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);
    std::normal_distribution<std::float64_t>       wind_dist(0.0, 2.5);
    std::normal_distribution<std::float64_t>       pressure_dist(101.325, 1.5);
    std::normal_distribution<std::float64_t>       temperature_dist(288.15, 5.0);
    std::normal_distribution<std::float64_t>       noise_altitude(0.0, 0.3);
    std::normal_distribution<std::float64_t>       noise_position(0.0, 0.5);

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
