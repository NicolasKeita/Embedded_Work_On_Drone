/*
Filename: Src/Simulation/PhysicsDispersion.cpp
Description: Implementation of the parameter dispersion mechanism for Monte Carlo simulations.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module PhysicsDispersion;

import std;

namespace
{
    sim::DispersionParameters current_parameters{};
}

namespace sim {

/* Returns a value snapshot of the startup distribution scales. */
DispersionParameters dispersion_parameters() noexcept
{
    return current_parameters;
}

/* Sets distribution defaults during startup, before concurrent campaign execution. */
void set_dispersion_parameters(const DispersionParameters& parameters) noexcept
{
    current_parameters = parameters;
}

/* Captures the configured scales while preserving the established seed derivation. */
DispersionGenerator::DispersionGenerator(std::uint64_t base_seed)
    : base_seed_{base_seed}
{
}

/* Derives the same stable independent seed for each campaign index. */
std::uint64_t DispersionGenerator::generate_run_seed(std::uint64_t base_seed, std::size_t run_index) const
{
    return base_seed + 2654435761ULL * static_cast<std::uint64_t>(run_index + 1);
}

/* Draws the configured dispersions in a fixed order for reproducible campaigns. */
PhysicsDispersion DispersionGenerator::generate_run_dispersion(std::size_t run_index) const
{
    PhysicsDispersion                              dispersion;
    const std::uint64_t                            run_seed = generate_run_seed(base_seed_, run_index);
    std::mt19937_64                                generator(run_seed);
    std::normal_distribution<std::float64_t>       normal_dist(0.0, 1.0);
    std::uniform_real_distribution<std::float64_t> uniform_minus1_to_1(-1.0, 1.0);
    std::uniform_real_distribution<std::float64_t> uniform_0_to_1(0.0, 1.0);
    std::uniform_real_distribution<std::float64_t> uniform_0_to_2pi(0.0, 2.0 * std::numbers::pi);

    dispersion.mass_variation = normal_dist(generator) * parameters_.mass_variation_std;
    dispersion.cog_offset_x = normal_dist(generator) * parameters_.cog_offset_x_std_m;
    dispersion.cog_offset_y = normal_dist(generator) * parameters_.cog_offset_y_std_m;
    dispersion.cog_offset_z = normal_dist(generator) * parameters_.cog_offset_z_std_m;
    dispersion.actuator_gain_dispersion = normal_dist(generator) * parameters_.actuator_gain_std;
    dispersion.actuator_lag_dispersion = std::abs(normal_dist(generator)) * parameters_.actuator_lag_std_s;
    dispersion.wind_speed_mean = std::abs(normal_dist(generator)) * parameters_.wind_speed_std_mps;
    dispersion.wind_heading_rad = uniform_0_to_2pi(generator);

    dispersion.turbulence_intensity = uniform_0_to_1(generator) * parameters_.turbulence_maximum;

    dispersion.atmospheric_density_offset = uniform_minus1_to_1(generator) * parameters_.atmospheric_density_maximum_offset;

    dispersion.atmospheric_pressure_offset = uniform_minus1_to_1(generator) * parameters_.atmospheric_pressure_maximum_offset;

    dispersion.imu_accel_noise_std = std::abs(normal_dist(generator)) * parameters_.imu_accel_noise_scale_mps2;

    dispersion.imu_gyro_noise_std = std::abs(normal_dist(generator)) * parameters_.imu_gyro_noise_scale_radps;

    dispersion.barometer_bias = normal_dist(generator) * parameters_.barometer_bias_std_m;

    dispersion.barometer_drift = normal_dist(generator) * parameters_.barometer_drift_std_mps;

    dispersion.gps_latency_jitter = std::abs(normal_dist(generator)) * parameters_.gps_latency_jitter_std_s;

    return dispersion;
}

}
