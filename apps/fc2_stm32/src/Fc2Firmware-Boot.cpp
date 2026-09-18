/*
Filename: apps/fc2_stm32/src/Fc2Firmware-Boot.cpp
Description: Shared mutable state of the FC2 firmware units (link bookkeeping, status LED)
and the boot sequence resolving the HIL console UART and the inter-FC USART.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

module Fc2Firmware;

import std;

import InterFcLink;
import StatusLed;

namespace fc2 {

Fc2Link inter_fc{};
FlightCore::Status::StatusLed status_led{};

/* Advances the status LED from the current supervision state. */
void update_status_led(FlightCore::InterFc::NodeState     node_state,
                       FlightCore::InterFc::DetectionCode detection) noexcept
{
    const bool led_fault = node_state != FlightCore::InterFc::NodeState::Healthy
        || detection != FlightCore::InterFc::DetectionCode::None;

    static_cast<void>(status_led.update(k_uptime_get(), led_fault));
}

Fc2Boot boot_fc2_hardware()
{
    Fc2Boot       boot{.uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console))};
    const device* inter_fc_uart = DEVICE_DT_GET(DT_NODELABEL(usart3));

    boot.inter_fc_transport = FlightCore::InterFc::ZephyrUartInterFcTransport{inter_fc_uart};
    inter_fc.startup_state = 1;
    if (!device_is_ready(boot.uart)) {
        return boot;
    }
    inter_fc.startup_state = 2;
    if (!boot.inter_fc_transport.ready()) {
        return boot;
    }
    inter_fc.startup_state = 3;
    boot.ready = true;
    return boot;
}

}
