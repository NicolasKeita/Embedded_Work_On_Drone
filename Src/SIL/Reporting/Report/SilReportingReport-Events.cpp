/*
Filename: Src/SIL/Reporting/Report/SilReportingReport-Events.cpp
Description: Important discrete events section of the Markdown report (no heartbeat noise).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReportingReport;

import std;

import SilEvents;

namespace sim::sil {

/*
Writes the important discrete events of one scenario section (no heartbeat or
per-message traffic).
*/
void write_event_table(std::ostream& out, std::span<const SilEvent> events)
{
    out << "| t(s) | Categorie | Evenement |\n|---|---|---|\n";
    for (const SilEvent& event : events) {
        if (!is_report_event(event.type)) {
            continue;
        }
        out << "| ";
        write_seconds(out, event.timestamp);
        out << " | " << event_category(event.type) << " | ";
        write_event_description(out, event);
        out << " |\n";
    }
}

}
