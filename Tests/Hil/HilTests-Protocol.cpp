/*
Filename: Tests/Hil/HilTests-Protocol.cpp
Description: HIL protocol test dispatch: drives the transport-layer frame tests and the
HilProtoTests module (inter-FC status, control setpoints and protocol version).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import HilProtoTests;
import TestHarness;

namespace sim::test::hil {

void run_protocol_tests(sim::test::TestHarness& runner)
{
    runner.set_context("PROTO");
    run_transport_tests(runner);
    run_proto_inter_fc_tests(runner);
    run_proto_setpoint_tests(runner);
    run_proto_dynamic_setpoint_tests(runner);
    run_proto_version_tests(runner);
}

}
