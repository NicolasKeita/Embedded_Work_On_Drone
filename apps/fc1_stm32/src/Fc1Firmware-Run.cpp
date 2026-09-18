/*
Filename: apps/fc1_stm32/src/Fc1Firmware-Run.cpp
Description: FC1 Zephyr entry point and main dispatch loop over the HIL console UART.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>

module Fc1Firmware;

import std;

import HilProtocol;
import HilProtocolParser;
import InterFcLink;
import StatusLed;

namespace fc1 {

namespace {
    RING_BUF_DECLARE(uart_receive_buffer, kUartReceiveCapacity);

    /* Collects the hardware handles of a booted FC1 unit. */
    struct Fc1Boot {
        FlightCore::InterFc::ZephyrUartInterFcTransport inter_fc_transport{nullptr};
        const device* uart = nullptr;
        bool ready = false;
    };

    /* Moves received UART bytes from the hardware FIFO into the static HIL ring buffer. */
    void receive_uart_bytes(const device* uart, void*) noexcept
    {
        std::array<std::uint8_t, 64> received_bytes{};

        while (uart_irq_update(uart) != 0 && uart_irq_is_pending(uart) != 0) {
            if (uart_irq_rx_ready(uart) == 0) {
                continue;
            }
            const std::int32_t received = uart_fifo_read(uart, received_bytes.data(), received_bytes.size());
            if (received > 0) {
                ring_buf_put(&uart_receive_buffer, received_bytes.data(), static_cast<std::uint32_t>(received));
            }
        }
    }

    /* Configures the HIL console UART (interrupt reception) and the inter-FC USART. */
    Fc1Boot boot_fc1_hardware()
    {
        Fc1Boot boot{.uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console))};

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
        if (uart_irq_callback_user_data_set(boot.uart, receive_uart_bytes, nullptr) != 0) {
            return boot;
        }
        uart_irq_rx_enable(boot.uart);
        inter_fc.startup_state = 4;
        boot.ready = true;
        return boot;
    }
}

/* Zephyr FC1 entry point. Incoming SensorPackets provide the 100 Hz release cadence. */
export int run_fc1_firmware()
{
    const auto led_initialized = status_led.initialize();

    if (!led_initialized.has_value()) {
        printk("[LED] initialization failed: %d\n", led_initialized.error());
    }
    const Fc1Boot boot = boot_fc1_hardware();
    if (!boot.ready) {
        return -1;
    }
    printk("[BOOT] firmware=fc1_stm32 role=FC1 board=nucleo_l476rg hil_baud=460800 "
           "inter_fc=USART3/PB10/PB11/115200\n");

    const Fc1Links links{.uart = boot.uart, .inter_fc_transport = &boot.inter_fc_transport};
    Fc1Control control{};
    std::int64_t last_heartbeat_ms = -kHeartbeatPeriodMs;
    std::int64_t last_report_ms = 0;

    while (true) {
        service_inter_fc_link(links, inter_fc.heartbeat_suppressed != 0, last_heartbeat_ms, last_report_ms);
        update_status_led();
        std::uint8_t byte = 0;
        if (ring_buf_get(&uart_receive_buffer, &byte, 1) == 1) {
            const bool complete = control.parser.processByte(byte, control.header, control.payload);
            if (complete) {
                process_sensor(links, control);
            }
        }
        else {
            k_sleep(K_MSEC(1));
        }
    }
}

}
