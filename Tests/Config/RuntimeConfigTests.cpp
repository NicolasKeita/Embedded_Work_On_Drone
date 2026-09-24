/*
Filename: Tests/Config/RuntimeConfigTests.cpp
Description: Host configuration parsing, HIL coherence checks, and captured physics defaults.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module RuntimeConfigTests;

import std;

import Aircraft;
import ConfigFile;
import HilConfig;
import HilRuntimeConfig;
import PhysicsDispersion;
import TestHarness;

namespace
{
    /* Parses a short in-memory document using the same reader as the runners. */
    std::expected<sim::config::Reader, std::string> parse(std::string_view text)
    {
        std::istringstream input{std::string{text}};
        return sim::config::parse_config_file(input, "runtime-config-test");
    }

    /* Checks accepted formatting, source defaults, strict syntax, and consumed keys. */
    void test_parser(sim::test::TestHarness& runner)
    {
        runner.set_context("CONFIG-PARSER");
        auto reader = parse("# header\r\n\tperiod = 1e-2 # seconds\r\nenabled = true\r\nlabel = bench A\n");
        runner.check(reader.has_value(), "comments, whitespace and CRLF are accepted");
        if (!reader) {
            return;
        }
        std::float64_t period = 0.25;
        std::uint64_t omitted_seed = 42;
        bool enabled = false;
        std::string label{};
        reader->number("period", period, 0.001, 1.0);
        reader->unsigned_integer("seed", omitted_seed, 0, 100);
        reader->boolean("enabled", enabled);
        reader->text("label", label);
        runner.check(reader->finish().has_value() && period == 0.01 && enabled && label == "bench A",
                     "typed values retain inline comments only as comments");
        runner.check(omitted_seed == 42 && reader->resolved_text().find("seed = 42\n") != std::string::npos,
                     "omitted defaults are preserved and included in the effective configuration");
        for (const std::string_view invalid : {"missing separator", "bad key = 1", "empty = # no value"}) {
            runner.check(!parse(invalid), "malformed assignments are rejected");
        }
        auto duplicate = parse("period = 0.01\nperiod = 0.02\n");
        if (duplicate) {
            duplicate->number("period", period, 0.001, 1.0);
        }
        runner.check(!duplicate || !duplicate->finish(), "duplicate keys are rejected");
        auto unknown = parse("peroid = 0.01\n");
        runner.check(unknown && !unknown->finish(), "unknown keys cannot silently use a default");
    }

    /* Checks finite numbers, range bounds, full integer consumption, and strict booleans. */
    void test_numeric_validation(sim::test::TestHarness& runner)
    {
        runner.set_context("CONFIG-TYPES");
        for (const std::string_view invalid : {"nan", "inf", "-inf", "1e999", "0.1s", "-1", "2"}) {
            sim::config::Reader reader{"number-test"};
            reader.add("value", std::string{invalid}, 1);
            std::float64_t value = 0.5;
            reader.number("value", value, 0.0, 1.0);
            runner.check(!reader.finish() && value == 0.5, "invalid number fails without mutating the destination");
        }
        for (const std::string_view invalid : {"18446744073709551616", "-1", "1.5", "1ms"}) {
            sim::config::Reader reader{"integer-test"};
            reader.add("value", std::string{invalid}, 1);
            std::uint64_t value = 7;
            reader.unsigned_integer("value", value, 0, std::numeric_limits<std::uint64_t>::max());
            runner.check(!reader.finish() && value == 7, "overflow, negative and partial integers are rejected");
        }
        sim::config::Reader maximum{"integer-maximum-test"};
        maximum.add("value", "18446744073709551615", 1);
        std::uint64_t maximum_value = 0;
        maximum.unsigned_integer("value", maximum_value, 0, std::numeric_limits<std::uint64_t>::max());
        runner.check(maximum.finish().has_value() && maximum_value == std::numeric_limits<std::uint64_t>::max(),
                     "the full uint64 range is accepted without precision loss");
        for (const std::string_view invalid : {"yes", "1", "TRUE"}) {
            sim::config::Reader reader{"boolean-test"};
            reader.add("value", std::string{invalid}, 1);
            bool value = false;
            reader.boolean("value", value);
            runner.check(!reader.finish() && !value, "booleans require the documented true or false spelling");
        }
    }

    /* Checks final HIL values after files, scenarios, and command-line overrides are applied. */
    void test_hil_validation(sim::test::TestHarness& runner)
    {
        runner.set_context("CONFIG-HIL");
        sim::hil::HilConfig config{};
        config.controller.hover_rpm = 815.527280820643;
        const sim::hil::HilConfig nominal = config;
        runner.check(sim::host::validate_hil_runtime_config(config).has_value(), "nominal loopback configuration is valid");
        config.dt_s = std::numeric_limits<std::float64_t>::quiet_NaN();
        runner.check(!sim::host::validate_hil_runtime_config(config), "nonfinite control period is rejected");
        config.dt_s = 0.0;
        runner.check(!sim::host::validate_hil_runtime_config(config), "zero control period is rejected");
        config = nominal;
        config.telemetry_rate_hz = 101.0;
        runner.check(!sim::host::validate_hil_runtime_config(config), "telemetry cannot exceed the control frequency");
        config.telemetry_rate_hz = std::numeric_limits<std::float64_t>::quiet_NaN();
        runner.check(!sim::host::validate_hil_runtime_config(config), "nonfinite telemetry frequency is rejected");
        config = nominal;
        config.transport_timeout_us = 10001;
        runner.check(!sim::host::validate_hil_runtime_config(config), "transport timeout cannot exceed the control period");
        config = nominal;
        config.heartbeat_timeout_s = 0.001;
        runner.check(!sim::host::validate_hil_runtime_config(config), "heartbeat timeout cannot be shorter than one step");
        config = nominal;
        config.controller.kp_altitude += 1.0;
        runner.check(sim::host::validate_hil_runtime_config(config).has_value(), "custom controller gains are allowed in loopback");
        config.interface_name = "/dev/unused-test-device";
        runner.check(!sim::host::validate_hil_runtime_config(config), "physical HIL rejects unsupported controller overrides");
        config = nominal;
        config.interface_name = "/dev/unused-test-device";
        config.dt_s = 0.02;
        runner.check(!sim::host::validate_hil_runtime_config(config), "physical HIL preserves the compiled control period");
    }

    /* Checks constructor snapshots and restores startup state before returning to other suites. */
    void test_physics_capture(sim::test::TestHarness& runner)
    {
        runner.set_context("CONFIG-PHYSICS");
        const sim::AircraftParameters saved_aircraft = sim::aircraft_parameters();
        const sim::DispersionParameters saved_dispersion = sim::dispersion_parameters();
        sim::set_aircraft_parameters(sim::AircraftParameters{});
        sim::set_dispersion_parameters(sim::DispersionParameters{});
        ::Aircraft original{};
        const sim::DispersionGenerator original_generator{42};
        const std::float64_t original_mass_draw = original_generator.generate_run_dispersion(0).mass_variation;
        runner.check(original.hover_rpm() == 815.527280820643, "nominal hover remains unchanged");
        sim::AircraftParameters changed_aircraft{};
        changed_aircraft.mass_kg = 2.4;
        changed_aircraft.maximum_rpm = 1500.0;
        sim::set_aircraft_parameters(changed_aircraft);
        ::Aircraft changed{};
        runner.check(std::abs(changed.hover_rpm() / original.hover_rpm() - std::sqrt(2.0)) < 1.0e-12,
                     "new aircraft derive hover from their captured mass");
        const ::ControlCommand command{.wing_rpm = 2000.0};
        original.set_command(command);
        changed.set_command(command);
        original.update(0.01);
        changed.update(0.01);
        runner.check(original.state().actual_rpm == 2000.0 && changed.state().actual_rpm == 1500.0,
                     "existing aircraft preserve their actuator limits when defaults change");
        sim::DispersionParameters changed_dispersion{};
        changed_dispersion.mass_variation_std = 0.0;
        sim::set_dispersion_parameters(changed_dispersion);
        const sim::DispersionGenerator changed_generator{42};
        runner.check(changed_generator.generate_run_dispersion(0).mass_variation == 0.0 && original_mass_draw != 0.0,
                     "new generators use the configured distribution scale");
        runner.check(original_generator.generate_run_dispersion(0).mass_variation == original_mass_draw,
                     "existing generators keep their captured scale and deterministic draw");
        sim::set_aircraft_parameters(saved_aircraft);
        sim::set_dispersion_parameters(saved_dispersion);
    }
}

/* Runs the isolated host configuration regressions through the existing assertion harness. */
void sim::test::config::run_runtime_config_tests(sim::test::TestHarness& runner)
{
    test_parser(runner);
    test_numeric_validation(runner);
    test_hil_validation(runner);
    test_physics_capture(runner);
}
