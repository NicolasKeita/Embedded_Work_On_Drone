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
    // Use a simple hash-like combination to generate a unique seed for each run
    // This ensures deterministic reproducibility across runs with the same base seed
    std::uint64_t combined = base_seed + 2654435761ULL * static_cast<std::uint64_t>(run_index + 1);
    return combined;
}

PhysicsDispersion DispersionGenerator::generate_run_dispersion(std::size_t run_index) const
{
    PhysicsDispersion dispersion;
    
    // Generate a unique seed for this specific run
    const std::uint64_t run_seed = generate_run_seed(base_seed_, run_index);
    std::mt19937_64 generator(run_seed);
    
    // Standard normal distribution for most parameters
    std::normal_distribution<std::float64_t> normal_dist(0.0, 1.0);
    
    // Uniform distributions for bounded parameters
    std::uniform_real_distribution<std::float64_t> uniform_minus1_to_1(-1.0, 1.0);
    std::uniform_real_distribution<std::float64_t> uniform_0_to_1(0.0, 1.0);
    std::uniform_real_distribution<std::float64_t> uniform_0_to_2pi(0.0, 2.0 * std::numbers::pi);
    
    // Aircraft parameter dispersions
    // Mass variation: +/- 10% (normal distribution, std dev = 5%)
    dispersion.mass_variation = normal_dist(generator) * 0.05; // 5% std dev -> ~95% within +/- 10%
    
    // Center of gravity offsets: small offsets in meters (normal distribution)
    dispersion.cog_offset_x = normal_dist(generator) * 0.05; // +/- ~0.1m typically
    dispersion.cog_offset_y = normal_dist(generator) * 0.05; // +/- ~0.1m typically  
    dispersion.cog_offset_z = normal_dist(generator) * 0.02; // +/- ~0.04m typically
    
    // Actuator gain dispersion: +/- 5% variation
    dispersion.actuator_gain_dispersion = normal_dist(generator) * 0.025; // 2.5% std dev -> ~95% within +/- 5%
    
    // Actuator lag dispersion: small additional latency (0 to 0.1 seconds)
    dispersion.actuator_lag_dispersion = std::abs(normal_dist(generator)) * 0.025; // typically 0 to ~0.05s
    
    // Environment parameter dispersions
    // Wind speed: mean wind with some variation
    dispersion.wind_speed_mean = std::abs(normal_dist(generator)) * 2.0; // typically 0-4 m/s
    
    // Wind heading: uniform in all directions
    dispersion.wind_heading_rad = uniform_0_to_2pi(generator);
    
    // Turbulence intensity: 0 to 0.5 (moderate turbulence)
    dispersion.turbulence_intensity = uniform_0_to_1(generator) * 0.5;
    
    // Atmospheric density offset: +/- 2%
    dispersion.atmospheric_density_offset = uniform_minus1_to_1(generator) * 0.02;
    
    // Atmospheric pressure offset: +/- 2%
    dispersion.atmospheric_pressure_offset = uniform_minus1_to_1(generator) * 0.02;
    
    // Sensor noise parameters
    // IMU accelerometer noise: typical values for MEMS sensors
    dispersion.imu_accel_noise_std = std::abs(normal_dist(generator)) * 0.05; // typically 0-0.1 m/s²
    
    // IMU gyroscope noise: typical values for MEMS sensors  
    dispersion.imu_gyro_noise_std = std::abs(normal_dist(generator)) * 0.005; // typically 0-0.01 rad/s
    
    // Barometer bias: small constant offset
    dispersion.barometer_bias = normal_dist(generator) * 0.1; // typically +/- 0.2m
    
    // Barometer drift: slow drift over time
    dispersion.barometer_drift = normal_dist(generator) * 0.001; // typically +/- 0.002 m/s
    
    // GPS latency jitter: additional variable latency
    dispersion.gps_latency_jitter = std::abs(normal_dist(generator)) * 0.01; // typically 0-0.02s
    
    return dispersion;
}

}