/*
Filename: Src/Embedded/Hil/HilRunnerMain.cpp
Description: Entry point of hil_runner : loads a HIL scenario, executes the real-time
closed-loop mission against the host FC emulator and prints the human-readable report.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import HilConfig;
import HilRunner;
import HilRunnerContext;
import HilScenarios;
import SilFaultScenario;

namespace {

void print_usage(std::string_view name)
{
    std::cout << "HIL runner : real-time closed-loop mission against the FC target.\n";
    std::cout << "Usage: " << name << " [options]\n";
    std::cout << "  --scenario <id>         HIL-001..HIL-005 (default HIL-001)\n";
    std::cout << "  --duration <s>          override mission duration in seconds\n";
    std::cout << "  --telemetry-period <s>  override human-readable report period (default 1 s)\n";
    std::cout << "  --seed <n>              random seed\n";
    std::cout << "  --noise <stddev>        sensor measurement noise std dev in meters\n";
    std::cout << "  --no-realtime           run as fast as possible (no wall-clock pacing)\n";
    std::cout << "  --clock <Monotonic|Fast> wall-clock source (default Monotonic; Fast for tests)\n";
    std::cout << "  --deadline <Warn|Fail|Abort>  deadline-miss policy (default Warn)\n";
    std::cout << "  --list                  list available scenarios\n";
}

void list_scenarios()
{
    std::cout << "HIL scenarios:\n";
    for (const sim::hil::HilScenarioRecord& s : sim::hil::HilScenarioCatalog::all()) {
        std::cout << "  " << s.id << " : " << s.description << "\n";
    }
}

std::optional<std::string> value_of(int argc, char** argv, int& i, std::string_view label)
{
    if (i + 1 >= argc) {
        std::cerr << "missing value for " << label << "\n";
        return std::nullopt;
    }
    return std::string{argv[++i]};
}

}

int main(int argc, char** argv)
{
    std::string scenario_id = "HIL-001";
    std::optional<std::float64_t> duration;
    std::optional<std::float64_t> telemetry_period;
    std::optional<std::uint64_t> seed;
    std::optional<std::float64_t> noise;
    std::string clock_name;
    std::string deadline_name;
    bool no_realtime = false;
    bool list_only = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--list") {
            list_only = true;
        }
        else if (arg == "--scenario") {
            if (const auto v = value_of(argc, argv, i, "--scenario")) { scenario_id = *v; }
        }
        else if (arg == "--duration") {
            if (const auto v = value_of(argc, argv, i, "--duration")) { duration = std::stod(*v); }
        }
        else if (arg == "--telemetry-period") {
            if (const auto v = value_of(argc, argv, i, "--telemetry-period")) { telemetry_period = std::stod(*v); }
        }
        else if (arg == "--seed") {
            if (const auto v = value_of(argc, argv, i, "--seed")) { seed = std::stoull(*v); }
        }
        else if (arg == "--noise") {
            if (const auto v = value_of(argc, argv, i, "--noise")) { noise = std::stod(*v); }
        }
        else if (arg == "--no-realtime") {
            no_realtime = true;
        }
        else if (arg == "--clock") {
            if (const auto v = value_of(argc, argv, i, "--clock")) { clock_name = *v; }
        }
        else if (arg == "--deadline") {
            if (const auto v = value_of(argc, argv, i, "--deadline")) { deadline_name = *v; }
        }
        else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (list_only) {
        list_scenarios();
        return 0;
    }

    const sim::hil::HilScenarioRecord* record = sim::hil::HilScenarioCatalog::find(scenario_id);
    sim::hil::HilConfig config = record ? record->config : sim::hil::hil_base_config();
    config.scenario_id = scenario_id;

    if (duration) { config.duration_s = *duration; }
    if (telemetry_period) { config.report_period_s = *telemetry_period; }
    if (seed) { config.seed = *seed; }
    if (noise) { config.sensor_noise_stddev = *noise; }
    if (no_realtime) { config.real_time_pacing = false; }
    if (clock_name == "Fast") { config.clock_kind = sim::hil::ClockKind::Fast; }
    else if (clock_name == "Monotonic") { config.clock_kind = sim::hil::ClockKind::Monotonic; }
    if (deadline_name == "Fail") { config.deadline_policy = sim::hil::DeadlinePolicy::Fail; }
    else if (deadline_name == "Abort") { config.deadline_policy = sim::hil::DeadlinePolicy::Abort; }
    else if (deadline_name == "Warn") { config.deadline_policy = sim::hil::DeadlinePolicy::Warn; }

    std::array<sim::sil::FaultScenario, 1> scenarios{record ? record->fault : sim::sil::FaultScenario{}};
    sim::hil::HilRunner runner{config};
    const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> outcome = runner.run(scenarios);
    if (!outcome.has_value()) {
        std::cerr << "HIL run failed\n";
        return 1;
    }

    sim::hil::HilRunner::writeReport(std::cout, *outcome);
    return (*outcome).result.test_verdict ? 0 : 1;
}
