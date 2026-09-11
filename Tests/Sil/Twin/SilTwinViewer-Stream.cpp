/*
Filename: Tests/Sil/Twin/SilTwinViewer-Stream.cpp
Description: Visible-rate replay and functional-mission streaming of the Digital Twin
viewer: publisher context, functional sample callback and scenario streamers.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilTwinViewer;

import std;

import Aircraft;
import FlightControllerTypes;
import MissionRunner;
import Scenarios;
import SilFaultScenario;
import SilTelemetry;
import TestHarness;
import TwinWebSocketPublisher;

namespace sim::test::sil {

/* Replays one completed SIL scenario to the localhost viewer at its recorded telemetry cadence. */
void stream_sil_twin(const sim::sil::ScenarioRecord& record)
{
    if (record.telemetry.empty()) {
        return;
    }

    sim::hil::TwinWebSocketPublisher publisher{};
    std::cout << "\n--- Digital Twin SIL live replay: ws://localhost:8765/twin ---" << std::endl;
    std::float64_t previous_time = record.telemetry.front().time;
    for (const sim::sil::TelemetrySample& sample : record.telemetry) {
        const std::float64_t delay_s = std::clamp(sample.time - previous_time,
                                                  std::float64_t{0.0},
                                                  std::float64_t{0.1});
        std::this_thread::sleep_for(std::chrono::duration<std::float64_t>{delay_s});
        std::ostringstream snapshot;
        write_snapshot(snapshot, record, sample);
        publisher.publish(snapshot.str());
        previous_time = sample.time;
    }
}

/* Publishes one compressed functional-mission sample using the shared TwinSnapshot schema. */
void publish_functional_sample(const sim::test::MissionViewerSample& sample, void* opaque_context)
{
    auto&                context = *static_cast<FunctionalViewerContext*>(opaque_context);
    const AircraftState& aircraft = sample.aircraft;
    const std::float64_t airspeed = std::hypot(aircraft.vx, aircraft.vy, aircraft.vz);
    std::ostringstream   out;

    out << std::setprecision(8) << "{\"source\":\"SIL\",\"time_s\":" << sample.time
        << ",\"aircraft\":{\"x_m\":" << aircraft.x << ",\"y_m\":" << aircraft.y
        << ",\"z_m\":" << aircraft.z << ",\"altitude_m\":" << aircraft.z
        << ",\"pitch_rad\":" << aircraft.pitch << ",\"roll_rad\":" << aircraft.roll
        << ",\"airspeed_ms\":" << airspeed << "},\"target\":{\"x_m\":" << sample.target.x
        << ",\"y_m\":" << sample.target.y << ",\"altitude_m\":" << sample.target.z
        << "},\"actuators\":{\"rotor_rpm\":" << aircraft.actual_rpm
        << ",\"left_servo_deg\":" << aircraft.actual_left_servo
        << ",\"right_servo_deg\":" << aircraft.actual_right_servo << "},\"mission\":";
    write_json_string(out, sim::control::mission_state_name(sample.mission));
    out << ",\"health\":\"HEALTHY\",\"safety_mode\":\"NORMAL\",\"active_fault\":null"
        << ",\"fc1\":{\"status\":\"ONLINE\",\"components\":";
    write_components(out, {}, false);
    out << "},\"fc2\":{\"status\":\"ONLINE\",\"components\":";
    write_components(out, {}, false);
    out << "},\"hil\":{\"loop_hz\":100,\"deadline_misses\":0},\"events\":[]}";

    context.publisher.publish(out.str());
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

/* Runs one functional SIL mission while streaming its compressed progress to the viewer. */
void run_functional_sil_twin(const sim::test::ScenarioEntry& entry,
                             sim::test::TestHarness&         runner,
                             std::float64_t                  hover_rpm)
{
    FunctionalViewerContext context{};

    std::cout << "\n--- Digital Twin SIL live stream: ws://localhost:8765/twin ---" << std::endl;
    sim::test::set_mission_viewer_observer(
        sim::test::MissionViewerObserver{.callback = publish_functional_sample, .context = &context});
    entry.run(runner, hover_rpm, {});
    sim::test::set_mission_viewer_observer({});
}

}
