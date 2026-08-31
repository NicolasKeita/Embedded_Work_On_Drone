/*
Filename: Src/SIL/Core/Events/SilEvents-Levels.cpp
Description: Event severity naming and verbosity classification.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilEvents;

import std;

namespace sim::sil {

/*
Human-readable name of an event severity for the trace artifacts.
*/
std::string_view event_severity_name(EventSeverity severity)
{
    switch (severity) {
    case EventSeverity::Info:
        return "INFO";
    case EventSeverity::Warning:
        return "WARNING";
    case EventSeverity::Error:
        return "ERROR";
    case EventSeverity::Debug:
        return "DEBUG";
    case EventSeverity::Trace:
        return "TRACE";
    }
    return "UNKNOWN";
}

/*
Verbosity level an event belongs to: warnings and errors are always kept.
*/
SilLogLevel event_log_level(EventSeverity severity)
{
    switch (severity) {
    case EventSeverity::Debug:
        return SilLogLevel::Debug;
    case EventSeverity::Trace:
        return SilLogLevel::Trace;
    case EventSeverity::Info:
    case EventSeverity::Warning:
    case EventSeverity::Error:
    default:
        return SilLogLevel::Info;
    }
}

}