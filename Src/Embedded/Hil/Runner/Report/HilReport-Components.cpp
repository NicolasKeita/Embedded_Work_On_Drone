/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport-Components.cpp
Description: Logical component health map of the TwinSnapshot for the Digital Twin viewer.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

namespace sim::hil {

/* Writes the stable logical component map consumed by the procedural FC view. */
void write_components(std::ostream& out, std::string_view affected, bool failed)
{
    constexpr std::array<std::string_view, 7> names{
        "mcu", "transport", "sensors", "control", "actuators", "supervision", "safety"};

    out << '{';
    for (std::size_t index = 0; index < names.size(); ++index) {
        if (index > 0) {
            out << ',';
        }
        write_json_string(out, names[index]);
        out << ':';
        const bool affected_now = names[index] == affected;
        write_json_string(out, affected_now ? (failed ? "FAILED" : "DEGRADED") : "HEALTHY");
    }
    out << '}';
}

}
