/*
Filename: Src/SIL/Core/Events/SilEvents-Recorder.cpp
Description: Trace filtering and recording implementations.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilEvents;

import std;

import Aircraft;

namespace sim::sil {

SilTrace::SilTrace(SilTraceConfig config) : config_{config} {}

/*
Records one event when its severity belongs to the configured verbosity level.
*/
void SilTrace::record(SilEvent event)
{
    if (events_.size() >= config_.max_events) {
        return;
    }
    if (event_log_level(event.severity) > config_.level) {
        return;
    }
    events_.push_back(std::move(event));
}

std::span<const SilEvent> SilTrace::events() const noexcept
{
    return events_;
}

std::vector<SilEvent> SilTrace::take_events() noexcept
{
    return std::move(events_);
}

}