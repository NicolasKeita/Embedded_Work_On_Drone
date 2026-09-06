/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport-Header.cpp
Description: Run header of the HIL report : scenario banner, host/target note,
configuration block and telemetry table column header.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

import HilConfig;
import HilRunnerContext;

namespace sim::hil {

void write_header(std::ostream& out, const HilConfig& cfg, bool fault_expected)
{
    out << "\n============================================\n";
    out << cfg.scenario_id << " : " << (fault_expected ? "fault injection scenario" : "nominal station keeping")
        << "\n";
    out << "============================================\n\n";
    if (!fault_expected) {
        out << "Host-target closed-loop validation (NO physical STM32 present).\n\n";
    }

    out << "Configuration\n";
    out << "  Duration              : " << std::fixed << std::setprecision(1) << cfg.duration_s << " s\n";
    out << "  Control period        : " << std::setprecision(3) << cfg.dt_s << " s\n";
    out << "  Control frequency     : " << std::setprecision(0) << std::round(1.0 / cfg.dt_s) << " Hz\n";
    out << "  Random seed           : " << cfg.seed << "\n";
    out << "  Target altitude       : " << std::fixed << std::setprecision(1) << cfg.target.z << " m\n";
    out << "  Sensor noise stddev   : " << std::setprecision(3) << cfg.sensor_noise_stddev << " m\n";
    const std::string_view policy = deadline_policy_name(cfg.deadline_policy);
    out << "  Real-time pacing      : " << yes_no(cfg.real_time_pacing) << " (" << policy << ")\n";
    out << "  FC target             : host emulator fc1_hil_host core (STM32 target is FUTURE)\n\n";

    out << "Mission (sensor stream : FC-observed; ground truth recorded separately)\n";
    write_table_header(out);
    out << std::flush;
}

}
