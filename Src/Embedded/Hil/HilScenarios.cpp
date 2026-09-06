/*
Filename: Src/Embedded/Hil/HilScenarios.cpp
Description: HIL scenario registry entries (HIL-001 nominal, HIL-002..HIL-005 faults).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilScenarios;

import std;

import HilConfig;
import SilFaultScenario;

namespace sim::hil {

HilConfig hil_base_config()
{
    HilConfig config{};
    config.scenario_id = "HIL-001";
    return config;
}

namespace {
    HilConfig named(std::string_view id)
    {
        HilConfig config = hil_base_config();
        config.scenario_id = id;
        return config;
    }

    const std::array<HilScenarioRecord, 5> kScenarios{{
        {
            "HIL-001",
            "Nominal station keeping (no fault)",
            named("HIL-001"),
            sim::sil::FaultScenario{},
        },
        {
            "HIL-002",
            "FC1_FAILURE during station keeping",
            named("HIL-002"),
            sim::sil::FaultScenario{.start_time = 5.0, .duration = 0.0, .fault_type = sim::sil::FaultType::FC1Failure},
        },
        {
            "HIL-003",
            "Communication loss during station keeping",
            named("HIL-003"),
            sim::sil::FaultScenario{.start_time = 5.0, .duration = 0.0, .fault_type = sim::sil::FaultType::CommunicationLoss},
        },
        {
            "HIL-004",
            "Sensor fault (altitude out of range) during station keeping",
            named("HIL-004"),
            sim::sil::FaultScenario{.start_time = 5.0,
                                    .duration = 20.0,
                                    .fault_type = sim::sil::FaultType::SensorFault,
                                    .parameters = {.corruption = sim::sil::SensorCorruptionMode::AltitudeOutOfRange,
                                                  .corrupted_altitude_m = 99999.0}},
        },
        {
            "HIL-005",
            "Actuator degradation (efficiency 0.6) during station keeping",
            named("HIL-005"),
            sim::sil::FaultScenario{.start_time = 5.0,
                                    .duration = 0.0,
                                    .fault_type = sim::sil::FaultType::ActuatorDegradation,
                                    .parameters = {.efficiency = 0.6}},
        },
    }};
}

std::span<const HilScenarioRecord> HilScenarioCatalog::all() noexcept
{
    return kScenarios;
}

const HilScenarioRecord* HilScenarioCatalog::find(std::string_view id) noexcept
{
    for (const HilScenarioRecord& scenario : kScenarios) {
        if (scenario.id == id) {
            return &scenario;
        }
    }
    return nullptr;
}

}
