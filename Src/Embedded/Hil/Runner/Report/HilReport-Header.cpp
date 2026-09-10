/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport-Header.cpp
Description: Run header of the HIL report : scenario banner, host/target note,
configuration block and telemetry table column header.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

import HilConfig;
import HilRunnerContext;

namespace sim::hil {

namespace {
    [[nodiscard]] std::string read_trimmed(const std::filesystem::path& path)
    {
        std::ifstream input{path};
        std::string   value{};

        std::getline(input, value);
        return value;
    }

    [[nodiscard]] bool is_stlink(const std::filesystem::path& device_path)
    {
        const std::string vendor = read_trimmed(device_path / "idVendor");
        const std::string product = read_trimmed(device_path / "product");

        return vendor == "0483" && (product.contains("STLink") || product.contains("ST-LINK"));
    }

    void write_probe_status(std::ostream& out, Stm32ProbeStatus status)
    {
        switch (status) {
        case Stm32ProbeStatus::Detected:
            out << "  Physical STM32 probe  : DETECTED (ST-LINK USB; running firmware not verified)\n";
            return;
        case Stm32ProbeStatus::NotDetected:
            out << "  Physical STM32 probe  : NOT DETECTED on the USB bus\n";
            return;
        case Stm32ProbeStatus::DetectionUnavailable:
            out << "  Physical STM32 probe  : UNKNOWN (USB detection unavailable)\n";
            return;
        }
    }
}

/* Detects an attached STM32 ST-LINK probe through the Linux USB sysfs inventory. */
Stm32ProbeStatus detect_stm32_probe()
{
    const std::filesystem::path usb_devices{"/sys/bus/usb/devices"};
    std::error_code             error{};

    if (!std::filesystem::is_directory(usb_devices, error)) {
        return Stm32ProbeStatus::DetectionUnavailable;
    }
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator{usb_devices, error}) {
        if (error) {
            return Stm32ProbeStatus::DetectionUnavailable;
        }
        if (is_stlink(entry.path())) {
            return Stm32ProbeStatus::Detected;
        }
    }
    if (error) {
        return Stm32ProbeStatus::DetectionUnavailable;
    }
    return Stm32ProbeStatus::NotDetected;
}

/* Writes the run header with separate execution-target and physical-probe status. */
void write_header(std::ostream& out, const HilConfig& cfg, bool fault_expected,
                  Stm32ProbeStatus probe_status)
{
    out << "\n============================================\n";
    out << cfg.scenario_id << " : " << (fault_expected ? "fault injection scenario" : "nominal station keeping")
        << "\n";
    out << "============================================\n\n";
    if (cfg.interface_name == "loopback") {
        out << "Host-emulator closed-loop validation; the physical STM32 is not used by this run.\n\n";
    }
    else {
        out << "Physical-target closed-loop validation over a serial HIL channel.\n\n";
    }

    out << "Configuration\n";
    out << "  Duration              : " << std::fixed << std::setprecision(1) << cfg.duration_s << " s\n";
    out << "  Control period        : " << std::setprecision(3) << cfg.dt_s << " s\n";
    out << "  Control frequency     : " << std::setprecision(0) << std::round(1.0 / cfg.dt_s) << " Hz\n";
    out << "  Random seed           : " << cfg.seed << "\n";
    out << "  Target altitude       : " << std::fixed << std::setprecision(1) << cfg.target.z << " m\n";
    out << "  Sensor noise stddev   : " << std::setprecision(3) << cfg.sensor_noise_stddev << " m\n";
    const std::string_view policy = deadline_policy_name(cfg.deadline_policy);
    out << "  Real-time pacing      : YES, monotonic steady clock (" << policy << ")\n";
    if (cfg.interface_name == "loopback") {
        out << "  FC execution target   : in-process host emulator (not the physical STM32)\n";
    }
    else {
        out << "  FC execution target   : physical STM32 over " << cfg.interface_name << "\n";
    }
    write_probe_status(out, probe_status);
    out << "  Firmware verification : "
        << (cfg.interface_name == "loopback" ? "NOT PERFORMED" : "HIL protocol response required") << "\n\n";

    out << "Mission (sensor stream : FC-observed; ground truth recorded separately)\n";
    write_table_header(out);
    out << std::flush;
}

}
