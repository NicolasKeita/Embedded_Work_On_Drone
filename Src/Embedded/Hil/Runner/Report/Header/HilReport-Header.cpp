/*
Filename: Src/Embedded/Hil/Runner/Report/Header/HilReport-Header.cpp
Description: Scenario banner, telemetry-table opening and STM32 ST-LINK probe
detection of the HIL run header.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

import HilConfig;
import HilRunnerContext;

namespace sim::hil {

namespace {

/* Reads one sysfs attribute, returning the empty string when unreadable. */
[[nodiscard]] std::string read_sysfs_trimmed(const std::filesystem::path& path)
{
    std::ifstream input{path};
    std::string   value{};

    std::getline(input, value);
    return value;
}

/* Reports whether a USB device entry exposes an ST-LINK probe identity. */
[[nodiscard]] bool is_stlink_probe(const std::filesystem::path& device_path)
{
    const std::string vendor = read_sysfs_trimmed(device_path / "idVendor");
    const std::string product = read_sysfs_trimmed(device_path / "product");

    return vendor == "0483" && (product.contains("STLink") || product.contains("ST-LINK"));
}

/* Scans the USB inventory, returning Detected at the first ST-LINK entry. */
Stm32ProbeStatus scan_usb_probes(const std::filesystem::path& usb_devices, std::error_code& error)
{
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator{usb_devices, error}) {
        if (error) {
            return Stm32ProbeStatus::DetectionUnavailable;
        }
        if (is_stlink_probe(entry.path())) {
            return Stm32ProbeStatus::Detected;
        }
    }
    return error ? Stm32ProbeStatus::DetectionUnavailable : Stm32ProbeStatus::NotDetected;
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
    return scan_usb_probes(usb_devices, error);
}

/* Writes the scenario banner and the host/physical-target note. */
void write_banner(std::ostream& out, const HilConfig& cfg, bool fault_expected)
{
    out << "\n============================================\n";
    out << cfg.scenario_id << " : " << (fault_expected ? "fault injection scenario" : "nominal station keeping")
        << "\n";
    out << "============================================\n\n";
    if (cfg.interface_name == "loopback") {
        out << "Host-emulator closed-loop validation; the physical STM32 is not used by this run.\n\n";
        return;
    }
    out << "Physical-target closed-loop validation over a serial HIL channel.\n\n";
}

/* Writes the run header with separate execution-target and physical-probe status. */
void write_header(std::ostream& out, const HilConfig& cfg, bool fault_expected,
                  Stm32ProbeStatus probe_status)
{
    write_banner(out, cfg, fault_expected);
    write_config_block(out, cfg);
    write_target_block(out, cfg, probe_status);

    out << "Mission (sensor stream : FC-observed; ground truth recorded separately)\n";
    write_table_header(out);
    out << std::flush;
}

}
