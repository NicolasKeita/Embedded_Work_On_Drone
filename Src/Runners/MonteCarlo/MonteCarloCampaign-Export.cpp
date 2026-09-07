/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign-Export.cpp
Description: CSV and JSON campaign report writers associating perturbed run inputs
with measured output metrics for every Monte-Carlo iteration.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MonteCarloCampaign;

import std;

import PhysicsDispersion;

namespace sim::monte_carlo {

namespace {

    /* Round-trippable precision for every exported floating-point value. */
    constexpr auto kPrecision = std::numeric_limits<std::float64_t>::max_digits10;

    /* Returns the PASS/FAIL status string of one run. */
    std::string_view run_status(const RunMetrics& metrics)
    {
        return metrics.passed ? "PASS" : "FAIL";
    }

    /* Writes the CSV header row with semicolon delimiters. */
    void write_csv_header(std::ostream& out)
    {
        out << "run_id;master_seed;run_seed;status;"
            << "mass_variation;cog_offset_x;cog_offset_y;cog_offset_z;"
            << "actuator_gain_dispersion;actuator_lag_dispersion;"
            << "wind_speed_mean;wind_heading_rad;turbulence_intensity;"
            << "atmospheric_density_offset;atmospheric_pressure_offset;"
            << "imu_accel_noise_std;imu_gyro_noise_std;barometer_bias;"
            << "barometer_drift;gps_latency_jitter;"
            << "overshoot;settling_time;steady_state_error;max_acceleration\n";
    }

    /* Writes one CSV row for a run record. */
    void write_csv_row(std::ostream& out, const RunRecord& record)
    {
        const sim::PhysicsDispersion& d = record.inputs.dispersion;
        out << record.inputs.run_id << ';' << record.inputs.master_seed << ';'
            << record.inputs.run_seed << ';' << run_status(record.metrics) << ';'
            << d.mass_variation << ';' << d.cog_offset_x << ';' << d.cog_offset_y << ';'
            << d.cog_offset_z << ';' << d.actuator_gain_dispersion << ';'
            << d.actuator_lag_dispersion << ';' << d.wind_speed_mean << ';'
            << d.wind_heading_rad << ';' << d.turbulence_intensity << ';'
            << d.atmospheric_density_offset << ';' << d.atmospheric_pressure_offset << ';'
            << d.imu_accel_noise_std << ';' << d.imu_gyro_noise_std << ';'
            << d.barometer_bias << ';' << d.barometer_drift << ';' << d.gps_latency_jitter << ';'
            << record.metrics.overshoot << ';' << record.metrics.settling_time << ';'
            << record.metrics.steady_state_error << ';' << record.metrics.max_acceleration << '\n';
    }

    /* Writes one JSON object for a run record, closing the object. */
    void write_json_record(std::ostream& out, const RunRecord& record, bool last)
    {
        const sim::PhysicsDispersion& d = record.inputs.dispersion;
        out << "  {\n";
        out << "    \"run_id\": " << record.inputs.run_id << ",\n";
        out << "    \"master_seed\": " << record.inputs.master_seed << ",\n";
        out << "    \"run_seed\": " << record.inputs.run_seed << ",\n";
        out << "    \"status\": \"" << run_status(record.metrics) << "\",\n";
        out << "    \"inputs\": {\n";
        out << "      \"mass_variation\": " << d.mass_variation << ",\n";
        out << "      \"cog_offset_x\": " << d.cog_offset_x << ",\n";
        out << "      \"cog_offset_y\": " << d.cog_offset_y << ",\n";
        out << "      \"cog_offset_z\": " << d.cog_offset_z << ",\n";
        out << "      \"actuator_gain_dispersion\": " << d.actuator_gain_dispersion << ",\n";
        out << "      \"actuator_lag_dispersion\": " << d.actuator_lag_dispersion << ",\n";
        out << "      \"wind_speed_mean\": " << d.wind_speed_mean << ",\n";
        out << "      \"wind_heading_rad\": " << d.wind_heading_rad << ",\n";
        out << "      \"turbulence_intensity\": " << d.turbulence_intensity << ",\n";
        out << "      \"atmospheric_density_offset\": " << d.atmospheric_density_offset << ",\n";
        out << "      \"atmospheric_pressure_offset\": " << d.atmospheric_pressure_offset << ",\n";
        out << "      \"imu_accel_noise_std\": " << d.imu_accel_noise_std << ",\n";
        out << "      \"imu_gyro_noise_std\": " << d.imu_gyro_noise_std << ",\n";
        out << "      \"barometer_bias\": " << d.barometer_bias << ",\n";
        out << "      \"barometer_drift\": " << d.barometer_drift << ",\n";
        out << "      \"gps_latency_jitter\": " << d.gps_latency_jitter << "\n";
        out << "    },\n";
        out << "    \"metrics\": {\n";
        out << "      \"overshoot\": " << record.metrics.overshoot << ",\n";
        out << "      \"settling_time\": " << record.metrics.settling_time << ",\n";
        out << "      \"steady_state_error\": " << record.metrics.steady_state_error << ",\n";
        out << "      \"max_acceleration\": " << record.metrics.max_acceleration << "\n";
        out << "    }\n";
        out << "  }" << (last ? "\n" : ",\n");
    }

}

/*
Writes the full campaign report as CSV (header + one row per run) into the given
stream. Each row associates run identity, seeds, status, every perturbed input
variable and every measured output metric.
*/
void write_campaign_csv(std::ostream& out, const CampaignStats& stats)
{
    out << std::setprecision(kPrecision);
    write_csv_header(out);
    for (const RunRecord& record : stats.records) {
        write_csv_row(out, record);
    }
}

/*
Writes the full campaign report as a JSON array (one object per run) into the
given stream, mirroring the CSV columns.
*/
void write_campaign_json(std::ostream& out, const CampaignStats& stats)
{
    out << std::setprecision(kPrecision);
    out << "[\n";
    const std::size_t count = stats.records.size();
    for (std::size_t index = 0; index < count; ++index) {
        write_json_record(out, stats.records[index], index + 1 == count);
    }
    out << "]\n";
}

/*
Writes the campaign CSV report to <path>; returns false and prints a diagnostic
to stderr when the file cannot be opened.
*/
bool export_campaign_csv(const CampaignStats& stats, std::string_view path)
{
    std::ofstream file{std::string{path}};
    if (!file) {
        std::cerr << "Error: cannot write CSV report to " << path << std::endl;
        return false;
    }
    write_campaign_csv(file, stats);
    return true;
}

/*
Writes the campaign JSON report to <path>; returns false and prints a diagnostic
to stderr when the file cannot be opened.
*/
bool export_campaign_json(const CampaignStats& stats, std::string_view path)
{
    std::ofstream file{std::string{path}};
    if (!file) {
        std::cerr << "Error: cannot write JSON report to " << path << std::endl;
        return false;
    }
    write_campaign_json(file, stats);
    return true;
}

}