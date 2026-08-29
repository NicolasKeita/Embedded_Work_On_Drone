/*
Filename: Src/SIL/SilReporting.cppm
Description: Export of SIL simulation results as Markdown, JSON and CSV artifacts.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilReporting;

import std;

import SilTypes;

export namespace sim::sil {

struct ScenarioRecord {
    std::string name;
    FaultScenario scenario;
    SimulationResult result;
};

struct SilReportOptions {
    std::filesystem::path docs_dir = "docs/validation";
    bool write_markdown = true;
    bool write_json = true;
    bool write_csv = true;
};

/*
Genere les artefacts de validation SIL (docs/validation/sil.md, sil.json,
sil.csv) et retourne le rapport Markdown.
*/
[[nodiscard]] std::string write_sil_report(const std::vector<ScenarioRecord>& records,
                                           const SilReportOptions& options = {});

[[nodiscard]] std::string json_payload(const std::vector<ScenarioRecord>& records);

[[nodiscard]] std::string csv_payload(const std::vector<ScenarioRecord>& records);

}
