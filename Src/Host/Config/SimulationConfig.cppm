/*
Filename: Src/Host/Config/SimulationConfig.cppm
Description: Host-only startup loading of aircraft model and Monte Carlo distribution parameters.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SimulationConfig;

import std;

export namespace sim::host
{
    /*
    Loads and validates simulation parameters, then installs numeric value defaults for
    subsequently constructed aircraft and dispersion generators. Call before starting
    simulation workers. Existing aircraft and generators keep their captured parameters.
    */
    [[nodiscard]] std::expected<void, std::string> load_simulation_config(const std::filesystem::path& path);
}
