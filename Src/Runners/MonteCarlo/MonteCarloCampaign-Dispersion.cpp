/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign-Dispersion.cpp
Description: Dispersion parameter printing for the verbose Monte-Carlo campaign output.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MonteCarloCampaign;

import std;

import PhysicsDispersion;

namespace sim::monte_carlo {

namespace {

    /* Prints one labelled dispersion parameter line with aligned columns. */
    void print_dispersion_field(std::string_view label, std::float64_t value, std::string_view unit)
    {
        std::cout << "      " << std::left << std::setw(26) << label << ": " << std::right
                  << std::setw(12) << std::setprecision(6) << value << ' ' << unit << std::endl;
    }
}

/* Prints the random dispersion parameters generated for one run. */
void print_dispersion(const sim::PhysicsDispersion& d,
                      std::uint64_t                 seed,
                      std::uint32_t                 run_index,
                      std::uint32_t                 run_count)
{
    std::cout << "  [Run " << (run_index + 1) << '/' << run_count << "] Dispersion (seed "
              << seed << ", run " << run_index << "):" << std::endl;
    std::cout << "    Aircraft:" << std::endl;
    print_dispersion_field("mass variation", d.mass_variation * 100.0, "%");
    print_dispersion_field("CoG offset x", d.cog_offset_x, "m");
    print_dispersion_field("CoG offset y", d.cog_offset_y, "m");
    print_dispersion_field("CoG offset z", d.cog_offset_z, "m");
    print_dispersion_field("actuator gain dispersion", d.actuator_gain_dispersion * 100.0, "%");
    print_dispersion_field("actuator lag dispersion", d.actuator_lag_dispersion, "s");
    std::cout << "    Environment:" << std::endl;
    print_dispersion_field("wind speed (mean)", d.wind_speed_mean, "m/s");
    print_dispersion_field("wind heading", d.wind_heading_rad * 180.0 / std::numbers::pi, "deg");
    print_dispersion_field("turbulence intensity", d.turbulence_intensity, "");
    print_dispersion_field("atmos. density offset", d.atmospheric_density_offset * 100.0, "%");
    print_dispersion_field("atmos. pressure offset", d.atmospheric_pressure_offset * 100.0, "%");
    std::cout << "    Sensors:" << std::endl;
    print_dispersion_field("IMU accel noise (std)", d.imu_accel_noise_std, "m/s^2");
    print_dispersion_field("IMU gyro noise (std)", d.imu_gyro_noise_std, "rad/s");
    print_dispersion_field("barometer bias", d.barometer_bias, "m");
    print_dispersion_field("barometer drift", d.barometer_drift, "m/s");
    print_dispersion_field("GPS latency jitter", d.gps_latency_jitter, "s");
}

}
