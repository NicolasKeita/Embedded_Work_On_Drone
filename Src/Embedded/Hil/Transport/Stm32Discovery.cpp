/*
Filename: Src/Embedded/Hil/Transport/Stm32Discovery.cpp
Description: Linux sysfs and serial-by-id implementation of trusted STM32 discovery.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Stm32Discovery;

import std;

namespace sim::hil {

namespace {
    constexpr std::string_view kStVendorId = "0483";

    [[nodiscard]] std::string read_trimmed(const std::filesystem::path& path)
    {
        std::ifstream input{path};
        std::string   value{};

        std::getline(input, value);
        return value;
    }

    [[nodiscard]] std::optional<std::filesystem::path>
    matching_usb_parent(std::filesystem::path path, std::string_view required_serial)
    {
        for (;;) {
            const std::string vendor = read_trimmed(path / "idVendor");
            const std::string serial = read_trimmed(path / "serial");
            if (vendor == kStVendorId && serial == required_serial) {
                return path;
            }
            const std::filesystem::path parent = path.parent_path();
            if (parent == path || path == "/sys") {
                return std::nullopt;
            }
            path = parent;
        }
    }

    [[nodiscard]] std::string stable_device_path(const std::filesystem::path& tty_path,
                                                  std::string_view required_serial)
    {
        const std::filesystem::path by_id{"/dev/serial/by-id"};
        const std::string serial_marker = std::format("_{}-if", required_serial);
        std::error_code error{};

        if (std::filesystem::is_directory(by_id, error)) {
            for (const std::filesystem::directory_entry& entry :
                 std::filesystem::directory_iterator{by_id, error}) {
                const std::string name = entry.path().filename().string();
                if (!name.starts_with("usb-STMicroelectronics_STM32_STLink_")
                    || !name.contains(serial_marker)) {
                    continue;
                }
                const std::filesystem::path target = std::filesystem::weakly_canonical(entry.path(), error);
                if (!error && target.filename() == tty_path.filename()) {
                    return entry.path().string();
                }
                error.clear();
            }
        }
        return (std::filesystem::path{"/dev"} / tty_path.filename()).string();
    }
}

/* Finds exactly one STMicroelectronics ACM port with the required ST-LINK serial. */
Stm32DiscoveryResult discover_stm32_port(std::string_view required_serial)
{
    const std::filesystem::path tty_class{"/sys/class/tty"};
    std::error_code             error{};
    std::string                 matched_path{};
    std::size_t                 match_count = 0;

    if (required_serial.empty() || !std::filesystem::is_directory(tty_class, error)) {
        return Stm32DiscoveryResult{.status = Stm32DiscoveryStatus::Unavailable};
    }
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator{tty_class, error}) {
        if (error) {
            return Stm32DiscoveryResult{.status = Stm32DiscoveryStatus::Unavailable};
        }
        if (!entry.path().filename().string().starts_with("ttyACM")) {
            continue;
        }
        const std::filesystem::path device = std::filesystem::weakly_canonical(entry.path() / "device", error);
        if (error) {
            error.clear();
            continue;
        }
        if (!matching_usb_parent(device, required_serial).has_value()) {
            continue;
        }
        ++match_count;
        matched_path = stable_device_path(entry.path(), required_serial);
    }
    if (match_count == 1) {
        return Stm32DiscoveryResult{.status = Stm32DiscoveryStatus::Matched,
                                    .device_path = std::move(matched_path)};
    }
    if (match_count > 1) {
        return Stm32DiscoveryResult{.status = Stm32DiscoveryStatus::Ambiguous};
    }
    return Stm32DiscoveryResult{.status = Stm32DiscoveryStatus::NotFound};
}

}
