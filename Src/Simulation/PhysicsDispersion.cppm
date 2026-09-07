/*
Filename: Src/Simulation/PhysicsDispersion.cppm
Description: Parameter dispersion mechanism for Monte Carlo simulations.
Exports:
    struct PhysicsDispersion,
    class DispersionGenerator

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module PhysicsDispersion;

import std;

export namespace sim {

/*
Aircraft and environment parameter dispersions for Monte Carlo simulations.
Contains variations for mass, center of gravity, actuators, environment, and sensors.
*/
struct PhysicsDispersion
{
    // Aircraft parameters
    std::float64_t mass_variation = 0.0;             // Mass variation percentage (-1.0 to +1.0 = +/- 100%)
    std::float64_t cog_offset_x = 0.0;              // Center of gravity offset in x direction (meters)
    std::float64_t cog_offset_y = 0.0;              // Center of gravity offset in y direction (meters)
    std::float64_t cog_offset_z = 0.0;              // Center of gravity offset in z direction (meters)
    std::float64_t actuator_gain_dispersion = 0.0;   // Actuator gain variation percentage
    std::float64_t actuator_lag_dispersion = 0.0;    // Actuator lag/latency variation (seconds)

    // Environment parameters
    std::float64_t wind_speed_mean = 0.0;           // Mean wind speed (m/s)
    std::float64_t wind_heading_rad = 0.0;          // Wind heading (radians)
    std::float64_t turbulence_intensity = 0.0;      // Turbulence intensity (0.0 to 1.0)
    std::float64_t atmospheric_density_offset = 0.0; // Atmospheric density offset percentage
    std::float64_t atmospheric_pressure_offset = 0.0; // Atmospheric pressure offset percentage

    // Sensor noise parameters
    std::float64_t imu_accel_noise_std = 0.0;       // IMU accelerometer noise standard deviation (m/s²)
    std::float64_t imu_gyro_noise_std = 0.0;        // IMU gyroscope noise standard deviation (rad/s)
    std::float64_t barometer_bias = 0.0;            // Barometer bias (meters)
    std::float64_t barometer_drift = 0.0;           // Barometer drift (meters/second)
    std::float64_t gps_latency_jitter = 0.0;        // GPS latency jitter (seconds)
};

/*
Generates deterministic physical parameter dispersions for Monte Carlo runs.
Uses std::mt19937_64 with a base seed and run index to produce reproducible random variations.
*/
class DispersionGenerator
{
public:
    explicit DispersionGenerator(std::uint64_t base_seed);

    /*
    Generates PhysicsDispersion for a specific run index.
    Uses normal distributions for most parameters and uniform distributions for bounded parameters.
    */
    [[nodiscard]] PhysicsDispersion generate_run_dispersion(std::size_t run_index) const;

private:
    std::uint64_t base_seed_;
    
    // Seed generation for a specific run
    [[nodiscard]] std::uint64_t generate_run_seed(std::uint64_t base_seed, std::size_t run_index) const;
};

}