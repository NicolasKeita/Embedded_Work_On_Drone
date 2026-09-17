/*
Filename: Src/Embedded/Hil/Config/HilScenarios.cppm
Description: Deterministic HIL scenario catalog. This module contains no scenario
definition of its own: every record is the HIL execution binding of one canonical
scenario of the shared FunctionalScenarios registry (identity, description and
mission profile). Only the fault activation timing, a property of the real-time
HIL bench, is defined here. The execution target (HIL) is injected at runtime, so
the scenario names never encode the execution environment.
Export summary: HilScenarioRecord, HilScenarioCatalog, hil_base_config()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilScenarios;

import std;

import HilConfig;
import SilFaultScenario;

export namespace sim::hil {

/*
One HIL execution binding: the canonical scenario identity and mission profile
(description) plus the derived HilConfig and the HIL-timed fault activation
(FaultScenario default when the scenario is nominal).
*/
struct HilScenarioRecord {
    std::string_view        id;
    std::string_view        description;
    HilConfig               config;
    sim::sil::FaultScenario fault;
};

/*
Base configuration shared by every HIL scenario: project control rate (100 Hz / 10 ms),
30 s nominal mission window, validated nominal target (10 m), structured telemetry and
conservative Warn deadline policy. Scenarios clone this and apply their mission profile.
*/
[[nodiscard]] HilConfig hil_base_config();

class HilScenarioCatalog {
public:
    [[nodiscard]] static std::span<const HilScenarioRecord> all() noexcept;
    [[nodiscard]] static const HilScenarioRecord* find(std::string_view id) noexcept;
};

}
