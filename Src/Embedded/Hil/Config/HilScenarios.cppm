/*
Filename: Src/Embedded/Hil/Config/HilScenarios.cppm
Description: Deterministic HIL scenario registry. Each scenario binds a HilRunner
configuration to a standardised functional scenario ID shared with the SIL
suite (NOMINAL-001 station keeping and the FAULT_INJECTOR-001..004 fault injection families),
reusing the SIL FaultScenario vocabulary so the same runner drives mission-level
and fault HIL tests. The execution target (HIL) is injected at runtime, so the
scenario names never encode the execution environment.
Export summary: HilScenarioRecord, HilScenarioCatalog, hil_base_config()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilScenarios;

import std;

import HilConfig;
import SilFaultScenario;

export namespace sim::hil {

struct HilScenarioRecord {
    std::string_view        id;
    std::string_view        description;
    HilConfig               config;
    sim::sil::FaultScenario fault;
};

/*
Base configuration shared by every HIL scenario: project control rate (100 Hz / 10 ms),
30 s nominal mission window, validated nominal target (10 m), structured telemetry and
conservative Warn deadline policy. Scenarios clone this and set their flight target and duration.
*/
[[nodiscard]] HilConfig hil_base_config();

class HilScenarioCatalog {
public:
    [[nodiscard]] static std::span<const HilScenarioRecord> all() noexcept;
    [[nodiscard]] static const HilScenarioRecord* find(std::string_view id) noexcept;
};

}
