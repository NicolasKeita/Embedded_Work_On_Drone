/*
Filename: Src/SIL/Reporting/SilReportFormat.cppm
Description: Shared formatting helpers and file writer for SIL validation artifacts.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilReportFormat;

import std;

export namespace sim::sil {

[[nodiscard]] std::string format_seconds(double value);

[[nodiscard]] std::string format_metric(double value);

[[nodiscard]] std::string json_escape(std::string_view text);

[[nodiscard]] std::string csv_escape(std::string_view text);

[[nodiscard]] std::string_view yes_no(bool value);

void write_file(const std::filesystem::path& path, std::string_view content);

}
