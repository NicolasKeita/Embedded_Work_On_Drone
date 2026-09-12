/*
Filename: Src/Embedded/InterFc/InterFcLink.cppm
Description: Transport-independent FC1-to-FC2 heartbeat protocol interface.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module InterFcLink;

import std;

export namespace FlightCore::InterFc {

enum class MessageKind : std::uint8_t {
    Heartbeat = 1,
    Acknowledgement = 2
};

enum class TransportError : std::uint8_t {
    DeviceUnavailable,
    TransmitFailed,
    ReceiveFailed
};

struct Message {
    MessageKind   kind = MessageKind::Heartbeat;
    std::uint16_t sequence = 0;
};

class IInterFcTransport {
public:
    virtual ~IInterFcTransport() = default;

    /* Sends one logical FC-to-FC message through the selected physical transport. */
    [[nodiscard]] virtual std::expected<void, TransportError> send(const Message& message) noexcept = 0;

    /* Polls for one complete logical message without blocking. */
    [[nodiscard]] virtual std::expected<std::optional<Message>, TransportError> poll() noexcept = 0;
};

class FrameCodec {
public:
    static constexpr std::size_t frame_size = 8;
    using Frame = std::array<std::uint8_t, frame_size>;

    /* Encodes one logical message into the UART framing shared by both controllers. */
    [[nodiscard]] static Frame encode(const Message& message) noexcept;
};

class FrameParser {
public:
    /* Consumes one byte and returns a message when a valid frame is complete. */
    [[nodiscard]] std::optional<Message> process(std::uint8_t byte) noexcept;

private:
    FrameCodec::Frame frame_{};
    std::size_t       size_ = 0;
};

}
