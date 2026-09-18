/*
Filename: Src/Embedded/Hil/Runner/HilRunner-Run.cpp
Description: Run orchestration of the HIL runner: live-stream and stop-flag registration,
execution of the configured scenarios into the structured outcome, and the
embedded-supervision reset performed before the serial port is released.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import Aircraft;
import HalTypes;
import HilClock;
import HilConfig;
import HilProtocol;
import HilRunnerContext;
import HilRunnerSafety;
import HilSensorModel;
import SilFaultScenario;
import Telemetry;

namespace sim::hil {

/* Restores a physical FC1/FC2 pair to nominal supervision before releasing the serial port. */
void reset_embedded_supervision(HilRunContext& ctx)
{
    if (!uses_embedded_fc2_supervision(ctx)) {
        return;
    }

    const AircraftState reset_truth = ctx.aircraft.state();
    const sim::sil::SensorTelemetry reset_telemetry = ctx.sensor_model.sample(reset_truth);
    const sim::sil::SensorValidity reset_validity = sim::sil::validate(reset_telemetry, ctx.config.sensor_limits);
    const FlightCore::HAL::SensorData reset_sensor = to_sensor_data(reset_telemetry, 0, reset_truth, reset_validity);
    const FlightCore::Transport::HilControlSetpoint setpoint{
        .target_x_m = static_cast<std::float32_t>(ctx.config.target.x),
        .target_y_m = static_cast<std::float32_t>(ctx.config.target.y),
        .target_z_m = static_cast<std::float32_t>(ctx.config.target.z),
        .station_hold_seconds = static_cast<std::float32_t>(ctx.config.controller.station_hold_seconds),
    };
    static_cast<void>(ctx.transport.sendSensor(reset_sensor, 0, setpoint));
    std::this_thread::sleep_for(std::chrono::milliseconds{150});
}

void HilRunner::setLiveStream(std::ostream& out)
{
    live_out_ = &out;
}

void HilRunner::setStopRequestedFlag(const volatile std::sig_atomic_t& stop_requested) noexcept
{
    stop_requested_ = &stop_requested;
}

/*
Runs the configured scenario(s) and returns the structured outcome, or a typed error.
*/
std::expected<HilRunOutput, HilError> HilRunner::run(std::span<const sim::sil::FaultScenario> scenarios)
{
    return makeContext(config_, scenarios).and_then([this](std::unique_ptr<HilRunContext> ctx) {
        ctx->live_out = live_out_;
        execute(*ctx);
        HilRunOutput output{.result = ctx->result,
                            .config = ctx->config,
                            .events = ctx->trace.takeEvents(),
                            .telemetry = std::move(ctx->telemetry_recorder.samples),
                            .ground_truth = std::move(ctx->telemetry_recorder.truth_samples)};

        return std::expected<HilRunOutput, HilError>{std::move(output)};
    });
}

}
