/*
Filename: Src/SIL/Reporting/SilReporting-Format.cpp
Description: Allocation-free JSON escaping helper for the SIL report export.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

namespace sim::sil {

/*
Streams the JSON-escaped text (quotes and backslashes) without building any
intermediate string.
*/
void write_json_escaped(std::ostream& out, std::string_view text)
{
    for (const char item : text) {
        if (item == '\"') {
            out << "\\\"";
        }
        else if (item == '\\') {
            out << "\\\\";
        }
        else {
            out << item;
        }
    }
}

}
