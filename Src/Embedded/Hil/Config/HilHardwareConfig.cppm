/*
Filename: Src/Embedded/Hil/Config/HilHardwareConfig.cppm
Description: File-based ST-LINK identities for the HIL hardware configuration.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilHardwareConfig;

import std;

export namespace sim::hil {

struct HilHardwareConfig {
    std::string fc1_stlink_serial{};
    std::string fc2_stlink_serial{};
};

/* Reads and validates the two distinct ST-LINK serial numbers from key=value text. */
[[nodiscard]] std::expected<HilHardwareConfig, std::string>
parse_hil_hardware_config(std::istream& input);

/* Loads a hardware configuration file and reports open, read, or validation errors. */
[[nodiscard]] std::expected<HilHardwareConfig, std::string>
load_hil_hardware_config(const std::filesystem::path& path);

}
