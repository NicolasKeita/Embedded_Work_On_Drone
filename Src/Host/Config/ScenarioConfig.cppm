/*
Filename: Src/Host/Config/ScenarioConfig.cppm
Description: Host-only startup loading of shared SIL/HIL scenario profiles.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module ScenarioConfig;

import std;

export namespace sim::host {

/*
Loads every canonical <scenario ID>.conf from the supplied directory, validates
the complete profile set, then updates the shared in-memory registry atomically.
*/
[[nodiscard]] std::expected<void, std::string> load_scenario_configs(
    const std::filesystem::path& directory);

}
