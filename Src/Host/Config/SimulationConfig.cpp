/*
Filename: Src/Host/Config/SimulationConfig.cpp
Description: Validated host configuration loading for aircraft dynamics and Monte Carlo scales.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SimulationConfig;

import std;

import Aircraft;
import ConfigFile;
import PhysicsDispersion;

namespace
{
    /* Loads physically meaningful model values while preserving omitted defaults. */
    void read_aircraft_parameters(sim::config::Reader& reader, sim::AircraftParameters& parameters)
    {
        reader.number("aircraft.mass_kg", parameters.mass_kg, 0.001, 10000.0);
        reader.number("aircraft.gravity_mps2", parameters.gravity_mps2, 0.001, 1000.0);
        reader.number("aircraft.lift_coefficient", parameters.lift_coefficient, 1.0e-12, 1.0e3);
        reader.number("aircraft.pitch_target_gain_rad_per_deg", parameters.pitch_target_gain_rad_per_deg, 0.0, 10.0);
        reader.number("aircraft.roll_target_gain_rad_per_deg", parameters.roll_target_gain_rad_per_deg, 0.0, 10.0);
        reader.number("aircraft.pitch_acceleration_gain_mps2_per_rad", parameters.pitch_acceleration_gain_mps2_per_rad, 0.0, 1000.0);
        reader.number("aircraft.roll_acceleration_gain_mps2_per_rad", parameters.roll_acceleration_gain_mps2_per_rad, 0.0, 1000.0);
        reader.number("aircraft.attitude_time_constant_s", parameters.attitude_time_constant_s, 1.0e-6, 1000.0);
        reader.number("aircraft.wind_gain_per_s", parameters.wind_gain_per_s, 0.0, 1000.0);
        reader.number("aircraft.minimum_rpm", parameters.minimum_rpm, 0.0, 1.0e6);
        reader.number("aircraft.maximum_rpm", parameters.maximum_rpm, 0.0, 1.0e6);
        reader.number("aircraft.minimum_servo_deg", parameters.minimum_servo_deg, -180.0, 180.0);
        reader.number("aircraft.maximum_servo_deg", parameters.maximum_servo_deg, -180.0, 180.0);
    }

    /* Loads distribution scales without changing draw order or seed mixing. */
    void read_dispersion_parameters(sim::config::Reader& reader, sim::DispersionParameters& parameters)
    {
        reader.number("dispersion.mass_variation_std", parameters.mass_variation_std, 0.0, 0.25);
        reader.number("dispersion.cog_offset_x_std_m", parameters.cog_offset_x_std_m, 0.0, 10.0);
        reader.number("dispersion.cog_offset_y_std_m", parameters.cog_offset_y_std_m, 0.0, 10.0);
        reader.number("dispersion.cog_offset_z_std_m", parameters.cog_offset_z_std_m, 0.0, 10.0);
        reader.number("dispersion.actuator_gain_std", parameters.actuator_gain_std, 0.0, 0.25);
        reader.number("dispersion.actuator_lag_std_s", parameters.actuator_lag_std_s, 0.0, 10.0);
        reader.number("dispersion.wind_speed_std_mps", parameters.wind_speed_std_mps, 0.0, 1000.0);
        reader.number("dispersion.turbulence_maximum", parameters.turbulence_maximum, 0.0, 1.0);
        reader.number("dispersion.atmospheric_density_maximum_offset", parameters.atmospheric_density_maximum_offset, 0.0, 0.99);
        reader.number("dispersion.atmospheric_pressure_maximum_offset", parameters.atmospheric_pressure_maximum_offset, 0.0, 0.99);
        reader.number("dispersion.imu_accel_noise_scale_mps2", parameters.imu_accel_noise_scale_mps2, 0.0, 1000.0);
        reader.number("dispersion.imu_gyro_noise_scale_radps", parameters.imu_gyro_noise_scale_radps, 0.0, 1000.0);
        reader.number("dispersion.barometer_bias_std_m", parameters.barometer_bias_std_m, 0.0, 10000.0);
        reader.number("dispersion.barometer_drift_std_mps", parameters.barometer_drift_std_mps, 0.0, 1000.0);
        reader.number("dispersion.gps_latency_jitter_std_s", parameters.gps_latency_jitter_std_s, 0.0, 1000.0);
    }

    /* Rejects inverted actuator limits and models whose nominal hover is unreachable. */
    std::expected<void, std::string> validate_aircraft_parameters(const sim::AircraftParameters& parameters)
    {
        if (parameters.minimum_rpm >= parameters.maximum_rpm) {
            return std::unexpected("aircraft.minimum_rpm must be lower than aircraft.maximum_rpm");
        }
        if (parameters.minimum_servo_deg >= parameters.maximum_servo_deg) {
            return std::unexpected("aircraft.minimum_servo_deg must be lower than aircraft.maximum_servo_deg");
        }
        const std::float64_t hover_rpm = std::sqrt(parameters.mass_kg * parameters.gravity_mps2 / parameters.lift_coefficient);
        if (hover_rpm < parameters.minimum_rpm || hover_rpm > parameters.maximum_rpm) {
            return std::unexpected("aircraft nominal hover RPM must lie within the configured actuator RPM limits");
        }
        return {};
    }
}

/* Loads host-side settings atomically with respect to model parameter installation. */
std::expected<void, std::string> sim::host::load_simulation_config(const std::filesystem::path& path)
{
    auto loaded = sim::config::load_config_file(path);
    if (!loaded) {
        return std::unexpected(loaded.error());
    }
    sim::AircraftParameters aircraft{};
    sim::DispersionParameters dispersion{};
    read_aircraft_parameters(*loaded, aircraft);
    read_dispersion_parameters(*loaded, dispersion);
    auto parsed = loaded->finish();
    if (!parsed) {
        return std::unexpected(parsed.error());
    }
    auto validated = validate_aircraft_parameters(aircraft);
    if (!validated) {
        return std::unexpected(path.string() + ": " + validated.error());
    }
    sim::set_aircraft_parameters(aircraft);
    sim::set_dispersion_parameters(dispersion);
    return {};
}
