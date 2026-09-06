/*
Filename: Src/Embedded/Hil/HilScenarios.cppm
Description: Deterministic HIL scenario registry. Each scenario binds a HilRunner
configuration to a declarative fault scenario (reusing the SIL FaultScenario vocabulary)
so the same runner drives mission-level and fault HIL tests. HIL-001 is the nominal
station-keeping mission; HIL-002 injects FC1_FAILURE during station keeping; HIL-003
communication loss, HIL-004 sensor fault and HIL-005 actuator degradation exercise the
other fault families.
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
    std::string_view          id;
    std::string_view          description;
    HilConfig                 config;
    sim::sil::FaultScenario   fault;
};

/*
Base configuration shared by every HIL scenario: project control rate (100 Hz / 10 ms),
30 s mission window, validated nominal target (10 m), 20 Hz structured telemetry, 1 Hz
report, conservative Warn deadline policy. Scenarios clone this and set their id/fault.
*/
[[nodiscard]] HilConfig hil_base_config();

class HilScenarioCatalog {
public:
    [[nodiscard]] static std::span<const HilScenarioRecord> all() noexcept;
    [[nodiscard]] static const HilScenarioRecord* find(std::string_view id) noexcept;
};

}
