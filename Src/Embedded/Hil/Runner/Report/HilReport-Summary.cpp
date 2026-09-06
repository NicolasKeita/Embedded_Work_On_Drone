/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport-Summary.cpp
Description: Post-run summary of the HIL report : timing and communication statistics,
mission result and verdict.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

import FlightController;
import HealthMonitor;
import HilRunnerContext;
import HilTiming;
import HilTransport;
import SafetyManager;

namespace sim::hil {

void write_summary(std::ostream& out, const HilRunOutput& output)
{
    const HilResult& r = output.result;

    out << "\nTiming\n";
    out << "  Steps executed        : " << r.timing.steps_executed << "\n";
    out << "  Deadline misses       : " << r.timing.deadline_misses << "\n";
    out << "  Max step duration     : " << r.timing.max_step_us << " us\n";
    out << "  Mean step duration    : " << static_cast<std::int64_t>(r.timing.mean_step_us) << " us\n";
    out << "  Max round trip        : " << r.timing.max_round_trip_us << " us\n";
    out << "  Max lateness          : " << r.timing.max_lateness_us << " us\n\n";

    out << "Communication statistics\n";
    out << "  Messages sent         : " << r.comms.messages_sent << "\n";
    out << "  Messages received     : " << r.comms.messages_received << "\n";
    out << "  Messages dropped      : " << r.comms.messages_dropped << "\n";
    out << "  Sequence errors       : " << r.comms.sequence_errors << "\n";
    out << "  Stale packets         : " << r.comms.stale_packets << "\n";
    out << "  Timeouts              : " << r.comms.timeouts << "\n";
    out << "  Latency min/mean/max  : " << r.comms.latency_min_us << " / "
        << static_cast<std::int64_t>(r.comms.latency_mean_us) << " / " << r.comms.latency_max_us << " us\n\n";

    out << "Mission result\n";
    out << "  Mission success       : " << yes_no(r.mission_success) << "\n";
    out << "  Final mission state   : " << sim::control::mission_state_name(r.final_state) << "\n";
    out << "  Final safety mode     : " << sim::safety::safety_mode_name(r.final_safety_mode) << "\n";
    out << "  Final health          : " << sim::safety::health_state_name(r.final_health) << "\n";
    out << "  Fault detected        : " << yes_no(r.fault_detected) << "\n";
    out << "  Detection latency     : " << std::fixed << std::setprecision(4) << r.detection_latency << " s\n";
    out << "  Test verdict          : " << (r.test_verdict ? "PASS" : "FAIL") << "\n";
    out << "  > " << hil_verdict_reason(r) << "\n";
}

}