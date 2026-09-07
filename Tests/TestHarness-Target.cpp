/*
Filename: Tests/TestHarness-Target.cpp
Description: Execution-target vocabulary of the shared validation harness (names, tags
and --target parsing).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module TestHarness;

import std;

namespace
{
    std::string_view lower_ascii(std::string_view text, std::string& buffer)
    {
        buffer.clear();
        buffer.reserve(text.size());
        for (char character : text) {
            buffer.push_back(static_cast<char>(character >= 'A' && character <= 'Z'
                                                  ? character + ('a' - 'A')
                                                  : character));
        }
        return buffer;
    }
}

namespace sim::test {

std::string_view run_target_name(RunTarget target) noexcept
{
    switch (target) {
    case RunTarget::SIL:        return "sil";
    case RunTarget::HIL:        return "hil";
    case RunTarget::Simulation: return "simulation";
    }
    return "simulation";
}

std::string_view run_target_tag(RunTarget target) noexcept
{
    switch (target) {
    case RunTarget::SIL:        return "SIL";
    case RunTarget::HIL:        return "HIL";
    case RunTarget::Simulation: return "SIM";
    }
    return "SIM";
}

std::expected<RunTarget, std::errc> parse_run_target(std::string_view name) noexcept
{
    std::string            lowered;
    const std::string_view normalised = lower_ascii(name, lowered);

    if (normalised == "sil" || normalised == "software-in-the-loop") {
        return RunTarget::SIL;
    }
    if (normalised == "hil" || normalised == "hardware-in-the-loop") {
        return RunTarget::HIL;
    }
    if (normalised == "sim" || normalised == "simulation" || normalised == "physics") {
        return RunTarget::Simulation;
    }
    return std::unexpected(std::errc::invalid_argument);
}

}
