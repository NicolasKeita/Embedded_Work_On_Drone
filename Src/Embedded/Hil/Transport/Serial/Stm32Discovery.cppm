/*
Filename: Src/Embedded/Hil/Transport/Serial/Stm32Discovery.cppm
Description: Trusted STM32 serial-port discovery using stable Linux USB identities.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Stm32Discovery;

import std;

export namespace sim::hil {

enum class Stm32DiscoveryStatus : std::uint8_t {
    Matched,
    NotFound,
    Ambiguous,
    Unavailable
};

struct Stm32DiscoveryResult {
    Stm32DiscoveryStatus status = Stm32DiscoveryStatus::Unavailable;
    std::string          device_path{};
};

/* Finds exactly one STMicroelectronics ACM port with the required ST-LINK serial. */
[[nodiscard]] Stm32DiscoveryResult discover_stm32_port(std::string_view required_serial);

}
