/*
Filename: Src/Embedded/Hil/Target/HilFcTarget-Host.cpp
Description: Host FC emulator target construction, SensorPacket reception/validation and
the Flight Controller cycle with last-good-sample hold.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilFcTarget;

import std;

import Aircraft;
import FlightController;
import FlightControllerTypes;
import HalTypes;
import HilClock;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import HilSensorModel;
import HilTransport;
import SimSensorInput;
import Telemetry;
import Transport;

namespace sim::hil {

HostFcTarget::HostFcTarget(FlightCore::Transport::ITransport& channel,
                            IWallClock& clock,
                            const sim::control::TargetState& target,
                            const sim::control::ControllerConfig& controller,
                            std::float64_t dt,
                            const sim::sil::SensorValidationLimits& sensor_limits)
    : channel_{channel},
      clock_{clock},
      fc_{controller},
      target_{target},
      dt_{dt},
      sensor_limits_{sensor_limits}
{
}

bool HostFcTarget::receive_sensor(std::uint16_t expected_sequence, FcStepOutcome& outcome,
                                  FlightCore::Transport::HilSensorPayload& out_payload)
{
    FlightCore::Transport::HilHeader header{};
    if (!receive_frame(channel_, parser_, header, payload_buffer_)) {
        return false;
    }
    if (header.msg_id != FlightCore::Transport::kMsgIdSensor
        || header.payload_len < FlightCore::Transport::kSensorPayloadSize) {
        return false;
    }
    if (header.sequence_num != expected_sequence) {
        return false;
    }

    outcome.fc_receive_wall_us = clock_.nowUs();

    const std::span<const std::uint8_t> span(payload_buffer_.data(),
                                             FlightCore::Transport::kSensorPayloadSize);
    if (!FlightCore::Transport::decodeSensorPayload(span, out_payload)) {
        return false;
    }
    outcome.sequence = header.sequence_num;
    outcome.echo_sim_timestamp_us = out_payload.sim_timestamp_us;
    return true;
}

ControlCommand HostFcTarget::update_control(const FlightCore::Transport::HilSensorPayload& sensor_payload)
{
    FlightCore::HAL::SensorData sensor = FlightCore::Transport::toSensorData(sensor_payload);
    const sim::sil::SensorTelemetry telemetry = to_telemetry(sensor);
    if (sim::sil::validate(telemetry, sensor_limits_).all_valid()) {
        sensor_input_.inject(sensor);
    }
    FlightCore::HAL::SensorData consumed{};
    if (sensor_input_.readSensorData(consumed)) {
        fc_view_ = to_aircraft_state(consumed);
        have_view_ = true;
    }
    if (!have_view_) {
        fc_view_ = AircraftState{};
    }

    return fc_.update(target_, fc_view_, dt_);
}

}