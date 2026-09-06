/*
Filename: Src/Embedded/Hil/HilRunner.cppm
Description: Public interface of the real-time HIL runner. The runner owns the
aircraft simulator, paces the closed loop on an absolute wall-clock deadline,
exchanges SensorPacket/ActuatorPacket with the FC target over the transport, records
truth/sensor telemetry and structured events, monitors deadlines and reuses the SIL
safety/health/comms/fault cores so detection matches the validated SIL baseline. It is
independent of the aircraft physics and the Flight Controller algorithm: those run in
the Aircraft module and in the FC target respectively.
Exports:
    class HilRunner

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilRunner;

import std;

import FlightController;
import HealthMonitor;
import HilConfig;
import HilEvents;
import HilRunnerContext;
import HilTiming;
import SilFaultScenario;

export namespace sim::hil {

class HilRunner {
public:
    explicit HilRunner(HilConfig config = {});

    /*
    Registers the stream the runner writes to in real time during run(): the mission
    telemetry rows (report cadence) interleaved with the structured event lines as they
    are recorded, each emission flushed immediately. When no stream is registered the
    run stays fully silent and reporting is left to writeReport().
    */
    void setLiveStream(std::ostream& out);

    /*
    Executes the configured scenario(s) and returns the structured run output, or a
    typed error. The run is real-time paced when config.real_time_pacing is set.
    */
    [[nodiscard]] std::expected<HilRunOutput, HilError> run(std::span<const sim::sil::FaultScenario> scenarios);

    /*
    Writes the human-readable HIL report (configuration, 1 Hz telemetry interleaved with
    the event timeline, communication/timing statistics, mission result and verdict).
    */
    static void writeReport(std::ostream& out, const HilRunOutput& output);

    /*
    Writes the run header : scenario banner, host/target note, configuration block and
    the telemetry table column header. Called before the run so the configuration is
    visible in real time while the mission is executing.
    */
    static void writeHeader(std::ostream& out, const HilConfig& config, bool fault_expected);

    /*
    Writes the post-run summary : timing, communication statistics, mission result and
    verdict. Only available once the run has completed.
    */
    static void writeSummary(std::ostream& out, const HilRunOutput& output);

private:
    [[nodiscard]] static std::expected<std::unique_ptr<HilRunContext>, HilError> makeContext(const HilConfig& config,
                                                                                               std::span<const sim::sil::FaultScenario> scenarios);
    static void execute(HilRunContext& ctx);
    static void finalize(HilRunContext& ctx);

    HilConfig config_;
    std::ostream* live_out_ = nullptr;
};

}

namespace sim::hil {

/*
Internal helpers shared across the HilRunner-*.cpp translation units (module-internal,
not exported to importers). They keep the loop, event recording, actuator application,
metrics, deadline handling and finalization factored into focused units.
*/
void record_run_start(HilRunContext& ctx);
void record_run_end(HilRunContext& ctx);
void record_mission_transition(HilRunContext& ctx, sim::control::MissionState current);
void record_safety_transitions(HilRunContext& ctx);
void record_detection(HilRunContext& ctx, const sim::safety::HealthReport& report);
void record_fault_events(HilRunContext& ctx);
void record_fc1_failure_event(HilRunContext& ctx);
void apply_injectors(HilRunContext& ctx);
void exchange_actuators(HilRunContext& ctx);
void update_health_and_safety(HilRunContext& ctx);
void apply_actuators(HilRunContext& ctx);
void update_metrics(HilRunContext& ctx);
void handle_deadline(HilRunContext& ctx, const HilStepTiming& timing);
void stream_live_output(HilRunContext& ctx);

}
