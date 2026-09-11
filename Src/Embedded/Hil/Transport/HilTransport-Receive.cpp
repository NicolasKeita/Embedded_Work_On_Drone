/*
Filename: Src/Embedded/Hil/Transport/HilTransport-Receive.cpp
Description: Deadline-bounded actuator receive of the HIL transport : frame validation
(message id, sequence, payload, sim-timestamp echo) and wall-clock round trip.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTransport;

import std;

import HalTypes;
import HilClock;
import HilProtocol;
import HilProtocolParser;
import Transport;

namespace sim::hil {

namespace {
    constexpr std::uint64_t kPollSleepUs = 100;
}

/*
Drains the channel through the parser until the ActuatorPacket answering the
expected sequence and echoed sim timestamp arrives, or the wall-clock deadline is
reached. Validates message id, sequence number and sim-timestamp echo so sequence,
duplicate and stale packets are caught; computes the wall-clock round trip.
*/
ReceiveResult HilTransport::receiveActuator(MonotonicClock&             clock,
                                            std::uint64_t               deadline_wall_us,
                                            const ActuatorExpectations& expectations,
                                            ActuatorReceiveOutputs      outputs)
{
    std::array<std::uint8_t, 128> rx{};

    for (;;) {
        while (channel_->bytesAvailable() > 0) {
            const std::size_t n = channel_->receiveBytes(rx);
            for (std::size_t i = 0; i < n; ++i) {
                FlightCore::Transport::HilHeader header{};
                const std::uint64_t rejected_before = parser_.rejectedFrames();
                if (parser_.processByte(rx[i], header, payload_buffer_)) {
                    const std::expected<ReceiveResult, FrameAcceptanceError> accepted =
                        accept_frame(header, expectations, clock.nowUs(), outputs);
                    if (accepted.has_value()) {
                        return accepted.value();
                    }
                }
                else if (parser_.rejectedFrames() > rejected_before) {
                    stats_.record_dropped();
                }
            }
        }
        if (clock.nowUs() >= deadline_wall_us) {
            stats_.record_timeout();
            return ReceiveResult::Timeout;
        }
        clock.sleepUntilUs(clock.nowUs() + kPollSleepUs);
    }
}

}
