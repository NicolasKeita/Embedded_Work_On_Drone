/*
Filename: Src/SIL/Reporting/Report/SilReportingReport-Telemetry.cpp
Description: Periodic telemetry and post-fault sections of the Markdown report.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReportingReport;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilTelemetry;
import SilTypes;

namespace sim::sil {

/*
Writes the full periodic mission telemetry table (used when no fault splits the
mission into pre/post windows).
*/
void write_telemetry_table(std::ostream&                    out,
                           std::span<const TelemetrySample> samples,
                           std::float64_t                   interval_s)
{
    write_telemetry_window(out, samples, 0.0, std::numeric_limits<std::float64_t>::max(), interval_s);
}

/*
Appends the telemetry/event/post-fault sections of one detailed scenario
section: mission telemetry, important events, then the post-fault window.
*/
void write_scenario_telemetry_sections(std::ostream&         out,
                                       const ScenarioRecord& record,
                                       std::float64_t        telemetry_report_interval_s)
{
    const std::float64_t fault_time = record.result.fault_injected_time;

    out << "\n#### Mission telemetry\n\n";
    const std::float64_t pre_fault_stop = fault_time < 0.0 ? std::numeric_limits<std::float64_t>::max() : fault_time;
    write_telemetry_window(out, record.telemetry, 0.0, pre_fault_stop, telemetry_report_interval_s);

    out << "\n#### Events\n\n";
    write_event_table(out, record.events);
    if (fault_time < 0.0) {
        out << "\n";
        return;
    }
    out << "\n#### Post-fault telemetry\n\n";
    write_telemetry_window(out, record.telemetry, fault_time, std::numeric_limits<std::float64_t>::max(),
                           telemetry_report_interval_s);
    out << "\n";
}

}
