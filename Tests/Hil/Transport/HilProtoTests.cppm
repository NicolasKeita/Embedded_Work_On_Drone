/*
Filename: Tests/Hil/Transport/HilProtoTests.cppm
Description: Interface of the HIL inter-FC protocol test module: inter-FC status frames,
control setpoints (static, dynamic and invalid) and protocol version negotiation, each
exercised through the real frame codec and parser.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilProtoTests;

import TestHarness;

export namespace sim::test::hil {

/* Inter-FC status round-trip tests (driven by the HIL protocol suite). */
void run_proto_inter_fc_tests(sim::test::TestHarness& runner);

/* Control setpoint round-trip and validation tests (driven by the HIL protocol suite). */
void run_proto_setpoint_tests(sim::test::TestHarness& runner);

/* Dynamic setpoint update tests (driven by the HIL protocol suite). */
void run_proto_dynamic_setpoint_tests(sim::test::TestHarness& runner);

/* Protocol version negotiation tests (driven by the HIL protocol suite). */
void run_proto_version_tests(sim::test::TestHarness& runner);

}
