/*
Filename: Tests/Hil/Config/HilHardwareConfigTests.cpp
Description: Regression tests for HIL hardware identity configuration parsing.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilHardwareConfigTests;

import std;

import HilHardwareConfig;
import TestHarness;

namespace sim::test::hil {
namespace {

struct InvalidHardwareConfigCase {
    std::string_view text;
    std::string_view label;
};

/* Verifies both board identities are preserved without requiring a final newline. */
void test_plain_hardware_config(sim::test::TestHarness& runner)
{
    std::istringstream input{
        "fc1_stlink_serial=066FFF525771555067225635\n"
        "fc2_stlink_serial=066CFF525771555067193653"};
    const auto parsed = sim::hil::parse_hil_hardware_config(input);
    runner.check(parsed.has_value(), "complete hardware config accepted without final newline");
    if (!parsed) {
        return;
    }
    runner.check(parsed->fc1_stlink_serial == "066FFF525771555067225635", "FC1 identity preserved");
    runner.check(parsed->fc2_stlink_serial == "066CFF525771555067193653", "FC2 identity preserved");
}

/* Verifies human-edited configuration formatting and case normalization. */
void test_formatted_hardware_config(sim::test::TestHarness& runner)
{
    std::istringstream input{
        " \t# Bench identities\r\n"
        "\r\n"
        "\tfc2_stlink_serial = 066cff525771555067193653 \t# supervision\r\n"
        " fc1_stlink_serial\t=\t066fFf525771555067225635 # control\r\n"
        "\t \r\n"};
    const auto parsed = sim::hil::parse_hil_hardware_config(input);
    runner.check(parsed.has_value(), "comments, whitespace, CRLF and reversed key order accepted");
    if (!parsed) {
        return;
    }
    runner.check(parsed->fc1_stlink_serial == "066FFF525771555067225635", "mixed-case FC1 identity normalized");
    runner.check(parsed->fc2_stlink_serial == "066CFF525771555067193653", "lowercase FC2 identity normalized");
}

/* Verifies invalid files fail with a diagnostic instead of yielding partial identities. */
void test_invalid_hardware_configs(sim::test::TestHarness& runner)
{
    constexpr std::array cases{
        InvalidHardwareConfigCase{
            "# no identities\n\n", "empty configuration rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial=066FFF525771555067225635\n", "missing FC2 identity rejected"},
        InvalidHardwareConfigCase{
            "fc2_stlink_serial=066CFF525771555067193653\n", "missing FC1 identity rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial=066FFF525771555067225635\n"
            "fc2_stlink_serial=066CFF525771555067193653\n"
            "fc3_stlink_serial=0123456789ABCDEF01234567\n", "unknown key rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial=066FFF525771555067225635\n"
            "fc2_stlink_serial=066CFF525771555067193653\n"
            "fc1_stlink_serial=066FFF525771555067225635\n", "duplicate FC1 key rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial=066FFF525771555067225635\n"
            "fc2_stlink_serial=066CFF525771555067193653\n"
            "fc2_stlink_serial=0123456789ABCDEF01234567\n", "duplicate FC2 key rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial 066FFF525771555067225635\n"
            "fc2_stlink_serial=066CFF525771555067193653\n", "missing equals sign rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial=\n"
            "fc2_stlink_serial=066CFF525771555067193653\n", "empty identity rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial=066FFF52577155506722563\n"
            "fc2_stlink_serial=066CFF525771555067193653\n", "short identity rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial=066FFF525771555067225635\n"
            "fc2_stlink_serial=066CFF5257715550671936530\n", "long identity rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial=066GFF525771555067225635\n"
            "fc2_stlink_serial=066CFF525771555067193653\n", "non-hexadecimal FC1 identity rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial=066FFF525771555067225635\n"
            "fc2_stlink_serial=066CFF52577155506719365Z\n", "non-hexadecimal FC2 identity rejected"},
        InvalidHardwareConfigCase{
            "fc1_stlink_serial=066FFF525771555067225635\n"
            "fc2_stlink_serial=066fff525771555067225635\n", "same board assigned to both roles rejected"},
    };
    for (const auto& test_case : cases) {
        std::istringstream input{std::string{test_case.text}};
        const auto parsed = sim::hil::parse_hil_hardware_config(input);
        runner.check(!parsed && !parsed.error().empty(), test_case.label);
    }
}

/* Verifies an unreadable stream produces a diagnostic. */
void test_failed_hardware_config_stream(sim::test::TestHarness& runner)
{
    std::istringstream input{
        "fc1_stlink_serial=066FFF525771555067225635\n"
        "fc2_stlink_serial=066CFF525771555067193653\n"};
    input.setstate(std::ios::badbit);
    const auto parsed = sim::hil::parse_hil_hardware_config(input);
    runner.check(!parsed && !parsed.error().empty(), "failed configuration stream rejected with a diagnostic");
}

}

/* Runs deterministic hardware identity configuration parser regressions. */
void run_hardware_config_tests(sim::test::TestHarness& runner)
{
    runner.set_context("HIL hardware config");
    test_plain_hardware_config(runner);
    test_formatted_hardware_config(runner);
    test_invalid_hardware_configs(runner);
    test_failed_hardware_config_stream(runner);
}

}
