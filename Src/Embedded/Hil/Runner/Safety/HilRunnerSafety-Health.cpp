/*
Filename: Src/Embedded/Hil/Runner/Safety/HilRunnerSafety-Health.cpp
Description: Health evaluation dispatch of the HIL runner : embedded physical FC2
report decoding, host-side HealthMonitor evaluation and safety-command recording.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerSafety;

import std;

import CommsBus;
import HealthMonitor;
import HilRunnerContext;
import HilRunnerEvents;
import InterFcLink;
import SafetyManager;
import SilTypes;

namespace sim::hil {

namespace {
    /* Decodes the physical FC2 detection identifier carried by FC1's HIL response. */
    std::optional<sim::safety::DetectionEvent> embedded_detection(std::uint8_t code) noexcept
    {
        switch (static_cast<FlightCore::InterFc::DetectionCode>(code)) {
        case FlightCore::InterFc::DetectionCode::Fc1HeartbeatTimeout:
            return sim::safety::DetectionEvent::FC1_HEARTBEAT_TIMEOUT;
        case FlightCore::InterFc::DetectionCode::CommunicationTimeout:
            return sim::safety::DetectionEvent::COMMUNICATION_TIMEOUT;
        case FlightCore::InterFc::DetectionCode::SensorValidationFailed:
            return sim::safety::DetectionEvent::SENSOR_VALIDATION_FAILED;
        case FlightCore::InterFc::DetectionCode::ActuatorMismatch:
            return sim::safety::DetectionEvent::ACTUATOR_MISMATCH;
        case FlightCore::InterFc::DetectionCode::None:
            break;
        }
        return std::nullopt;
    }

    /* Converts the health status physically reported by FC2 into the shared safety report. */
    sim::safety::HealthReport embedded_fc2_report(const HilRunContext& ctx) noexcept
    {
        sim::safety::HealthReport report{};
        const FlightCore::InterFc::NodeState state =
            static_cast<FlightCore::InterFc::NodeState>(ctx.actuator_diagnostics.fc_health_status);

        if (state == FlightCore::InterFc::NodeState::Safe) {
            report.state = sim::safety::HealthState::SAFE;
        }
        else if (state == FlightCore::InterFc::NodeState::Degraded) {
            report.state = sim::safety::HealthState::DEGRADED;
        }
        const std::optional<sim::safety::DetectionEvent> detection =
            embedded_detection(ctx.actuator_diagnostics.fc_detection_code);
        if (detection.has_value()) {
            const std::size_t index = static_cast<std::size_t>(*detection);
            report.flags[index] = sim::safety::DetectionFlag{.raised = true, .raised_time = ctx.time};
        }
        return report;
    }
}

/* Selects physical FC2 supervision whenever the HIL target is a serial STM32. */
bool uses_embedded_fc2_supervision(const HilRunContext& ctx) noexcept
{
    return ctx.config.interface_name != "loopback";
}

/* Evaluates host or embedded FC2 health and records the resulting safety command. */
void update_health_and_safety(HilRunContext& ctx)
{
    const sim::safety::HealthReport report = uses_embedded_fc2_supervision(ctx)
        ? embedded_fc2_report(ctx)
        : ctx.health.evaluate(ctx.time, ctx.comms, ctx.sensors, ctx.commanded_rpm);

    ctx.safety_command = ctx.safety.update(ctx.time, report);
    record_detection(ctx, report);
    record_safety_transitions(ctx);

    const bool degraded = report.state == sim::safety::HealthState::DEGRADED;
    const bool compensated = ctx.safety.mode() == sim::safety::SafetyMode::COMPENSATED;
    const bool safe_mode = ctx.safety.mode() == sim::safety::SafetyMode::SAFE_MODE;

    ctx.result.final_health = report.state;
    ctx.result.final_safety_mode = ctx.safety.mode();
    ctx.result.degraded_reached = ctx.result.degraded_reached || degraded;
    ctx.result.compensated_reached = ctx.result.compensated_reached || compensated;
    ctx.result.safe_mode_reached = ctx.result.safe_mode_reached || safe_mode;
}

}