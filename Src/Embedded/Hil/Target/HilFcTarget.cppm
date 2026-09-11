/*
Filename: Src/Embedded/Hil/Target/HilFcTarget.cppm
Description: Flight Controller target abstraction of the HIL runner. HostFcTarget runs
the REAL sim::control::FlightController core behind the embedded HAL abstractions;
replacing it with the physical STM32 only swaps this implementation, never the runner.
Exports:
    struct FcStepOutcome,
    class IFcTarget,
    class HostFcTarget,
    class RemoteFcTarget

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilFcTarget;

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
import SimActuatorOutput;
import SimSensorInput;
import Telemetry;
import Transport;

export namespace sim::hil {

struct FcStepOutcome {
    bool          ok = false;
    std::uint64_t fc_receive_wall_us = 0;
    std::uint64_t fc_send_wall_us = 0;
    std::uint16_t sequence = 0;
    std::uint64_t echo_sim_timestamp_us = 0;
    std::uint8_t  mission_state = 0;
};

class IFcTarget {
public:
    IFcTarget() = default;
    virtual ~IFcTarget() = default;

    IFcTarget(const IFcTarget&)            = delete;
    IFcTarget& operator=(const IFcTarget&) = delete;

    /*
    Processes the SensorPacket already on the channel for expected_sequence: drains it,
    runs one Flight Controller cycle and writes the answering ActuatorPacket back.
    Returns the wall-clock instants measured on the target side; for a future remote
    target these are host-side best-effort (the ActuatorPacket fc_timestamp field is the
    authoritative target clock).
    */
    virtual FcStepOutcome respond(std::uint16_t expected_sequence) = 0;
};

/* Physical target marker: the MCU responds asynchronously through the serial channel. */
class RemoteFcTarget final : public IFcTarget {
public:
    FcStepOutcome respond(std::uint16_t expected_sequence) override;
};

/* Bundles the Flight Controller configuration passed to the host emulator target. */
struct HostFcTargetConfig {
    sim::control::TargetState        target{};
    sim::control::ControllerConfig   controller{};
    std::float64_t                   dt = 0.0;
    sim::sil::SensorValidationLimits sensor_limits{};
};

/*
Host FC emulator target: the real Flight Controller core exercised in-process over
the shared byte channel. This is NOT the physical target; it exists only to drive the
HIL runner before the STM32 arrives and shares the FC algorithm with the future
fc1_stm32 build.
*/
class HostFcTarget final : public IFcTarget {
public:
    HostFcTarget(FlightCore::Transport::ITransport& channel,
                 MonotonicClock& clock,
                 const HostFcTargetConfig& config);

    FcStepOutcome respond(std::uint16_t expected_sequence) override;

private:
    /* Drains and validates the next SensorPacket; fills the receive-side outcome fields. */
    bool receive_sensor(std::uint16_t expected_sequence, FcStepOutcome& outcome,
                        FlightCore::Transport::HilSensorPayload& out_payload);

    /* Validates/injects the sensor sample, holds the last good view and runs the FC cycle. */
    ControlCommand update_control(const FlightCore::Transport::HilSensorPayload& sensor_payload);

    /* Emits the answering ActuatorPacket through the HAL actuator output and the channel. */
    bool send_actuators(const ControlCommand& command, FcStepOutcome& outcome);

    FlightCore::Transport::ITransport&                           channel_;
    MonotonicClock&                                              clock_;
    sim::control::FlightController                               fc_;
    FlightCore::Sim::SimulatedSensorInput                        sensor_input_;
    FlightCore::Sim::SimulatedActuatorOutput                     actuator_output_;
    FlightCore::Transport::HilFrameParser                        parser_{};
    std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> payload_buffer_{};
    sim::control::TargetState                                    target_;
    std::float64_t                                               dt_;
    sim::sil::SensorValidationLimits                             sensor_limits_;
    AircraftState                                                fc_view_{};
    bool                                                         have_view_ = false;
};

}
