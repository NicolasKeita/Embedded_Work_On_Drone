/*
Filename: Src/Simulation/PhysicsDispersion.cpp
Description: Implementation of the parameter dispersion mechanism for Monte Carlo simulations.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module PhysicsDispersion;

import std;

namespace sim {

DispersionGenerator::DispersionGenerator(std::uint64_t base_seed)
    : base_seed_{base_seed}
{
}

std::uint64_t DispersionGenerator::generate_run_seed(std::uint64_t base_seed, std::size_t run_index) const
{
    return base_seed + 2654435761ULL * static_cast<std::uint64_t>(run_index + 1);
}

PhysicsDispersion DispersionGenerator::generate_run_dispersion(std::size_t run_index) const
{
    PhysicsDispersion                              dispersion;
    const std::uint64_t                            run_seed = generate_run_seed(base_seed_, run_index);
    std::mt19937_64                                generator(run_seed);
    std::normal_distribution<std::float64_t>       normal_dist(0.0, 1.0);
    std::uniform_real_distribution<std::float64_t> uniform_minus1_to_1(-1.0, 1.0);
    std::uniform_real_distribution<std::float64_t> uniform_0_to_1(0.0, 1.0);
    std::uniform_real_distribution<std::float64_t> uniform_0_to_2pi(0.0, 2.0 * std::numbers::pi);

    dispersion.mass_variation = normal_dist(generator) * 0.05;
    dispersion.cog_offset_x = normal_dist(generator) * 0.05;
    dispersion.cog_offset_y = normal_dist(generator) * 0.05;
    dispersion.cog_offset_z = normal_dist(generator) * 0.02;
    dispersion.actuator_gain_dispersion = normal_dist(generator) * 0.025;
    dispersion.actuator_lag_dispersion = std::abs(normal_dist(generator)) * 0.025;
    dispersion.wind_speed_mean = std::abs(normal_dist(generator)) * 2.0;
    dispersion.wind_heading_rad = uniform_0_to_2pi(generator);

    dispersion.turbulence_intensity = uniform_0_to_1(generator) * 0.5;

    dispersion.atmospheric_density_offset = uniform_minus1_to_1(generator) * 0.02;

    dispersion.atmospheric_pressure_offset = uniform_minus1_to_1(generator) * 0.02;

    dispersion.imu_accel_noise_std = std::abs(normal_dist(generator)) * 0.05;

    dispersion.imu_gyro_noise_std = std::abs(normal_dist(generator)) * 0.005;

    dispersion.barometer_bias = normal_dist(generator) * 0.1;

    dispersion.barometer_drift = normal_dist(generator) * 0.001;

    dispersion.gps_latency_jitter = std::abs(normal_dist(generator)) * 0.01;

    return dispersion;
}

}
