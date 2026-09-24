/*
Filename: Src/Embedded/Hil/Runner/Report/Header/HilReport-Config.cpp
Description: Configuration block of the HIL run header.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

import HilConfig;
import HilRunnerContext;

namespace sim::hil {

/* Writes the single physical-probe status line of the run header. */
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

/* Writes the AUTO fallback wording matching a discovery outcome. */
void write_auto_fallback(std::ostream& out, InterfaceSelection selection)
{
    if (selection == InterfaceSelection::AutoFallbackAmbiguous) {
        out << "  Target selection      : AUTO, ambiguous trusted identity; loopback fallback\n";
        return;
    }
    if (selection == InterfaceSelection::AutoFallbackUnavailable) {
        out << "  Target selection      : AUTO, discovery unavailable; loopback fallback\n";
        return;
    }
    out << "  Target selection      : AUTO, trusted FC1 absent; loopback fallback\n";
}

/* Writes the target-selection line of the run header. */
void write_interface_selection(std::ostream& out, const HilConfig& config)
{
    switch (config.interface_selection) {
    case InterfaceSelection::ExplicitLoopback:
        out << "  Target selection      : explicit loopback\n";
        return;
    case InterfaceSelection::ExplicitSerial:
        out << "  Target selection      : explicit trusted FC1 device\n";
        return;
    case InterfaceSelection::AutoTrustedStm32:
        out << "  Target selection      : AUTO, trusted FC1 matched\n";
        return;
    case InterfaceSelection::RejectedUntrustedDevice:
        out << "  Target selection      : untrusted serial device rejected; loopback fallback\n";
        return;
    default:
        write_auto_fallback(out, config.interface_selection);
        return;
    }
}

/* Writes the numeric configuration block of the run header. */
void write_config_block(std::ostream& out, const HilConfig& cfg)
{
    out << "Configuration\n";
    out << "  Duration              : " << std::fixed << std::setprecision(1) << cfg.duration_s << " s\n";
    out << "  Control period        : " << std::setprecision(3) << cfg.dt_s << " s\n";
    out << "  Control frequency     : " << std::setprecision(0) << std::round(1.0 / cfg.dt_s) << " Hz\n";
    out << "  Random seed           : " << cfg.seed << "\n";
    out << "  Target altitude       : " << std::fixed << std::setprecision(1) << cfg.target.z << " m\n";
    out << "  Sensor noise stddev   : " << std::setprecision(3) << cfg.sensor_noise_stddev << " m\n";
}

/* Writes the target, probe and firmware lines closing the configuration. */
void write_target_block(std::ostream& out, const HilConfig& cfg, Stm32ProbeStatus probe_status)
{
    const std::string_view policy = deadline_policy_name(cfg.deadline_policy);

    out << "  Real-time pacing      : YES, monotonic steady clock (" << policy << ")\n";
    write_interface_selection(out, cfg);
    if (!cfg.fc1_stlink_serial.empty()) {
        out << "  Authorized FC1        : ST-LINK " << cfg.fc1_stlink_serial << "\n";
    }
    if (!cfg.fc2_stlink_serial.empty()) {
        out << "  Configured FC2        : ST-LINK " << cfg.fc2_stlink_serial << "\n";
    }
    if (cfg.interface_name == "loopback") {
        out << "  FC execution target   : in-process host emulator (not the physical STM32)\n";
    }
    else {
        out << "  FC execution target   : physical STM32 over " << cfg.interface_name << "\n";
    }
    write_probe_status(out, probe_status);
    out << "  Firmware verification : "
        << (cfg.interface_name == "loopback" ? "NOT PERFORMED" : "HIL protocol response required") << "\n\n";
}

}
