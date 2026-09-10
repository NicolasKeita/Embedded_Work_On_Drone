/*
Filename: Tests/Sil/SilTwinViewer.cppm
Description: Interface of the single-scenario SIL telemetry stream for the localhost Digital Twin viewer.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilTwinViewer;

import std;

import SilReporting;
import Scenarios;
import TestHarness;

export namespace sim::test::sil {

/* Replays one completed SIL scenario to the localhost viewer at its recorded telemetry cadence. */
void stream_sil_twin(const sim::sil::ScenarioRecord& record);

/* Runs one functional SIL mission while streaming its compressed progress to the viewer. */
void run_functional_sil_twin(const sim::test::ScenarioEntry& entry,
                             sim::test::TestHarness& runner,
                             std::float64_t hover_rpm);

}
