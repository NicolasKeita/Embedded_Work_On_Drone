/*
Filename: Src/Embedded/Hil/Runner/HilRunner.cppm
Description: Public interface of the real-time HIL runner. The runner owns the
aircraft simulator, paces the closed loop on an absolute wall-clock deadline,
exchanges SensorPacket/ActuatorPacket with the FC target over the transport, records
truth/sensor telemetry and structured events, monitors deadlines and reuses the SIL
safety/health/comms/fault cores so detection matches the validated SIL baseline. It is
independent of the aircraft physics and the Flight Controller algorithm: those run in
the Aircraft module and in the FC target respectively. Event recording lives in the
HilRunnerEvents module, human-readable reporting in the HilReport module.
Exports:
    class HilRunner

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilRunner;

import std;

import HilConfig;
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
    run stays fully silent and reporting is left to the HilReport module.
    */
    void setLiveStream(std::ostream& out);

    /*
    Executes the configured scenario(s) and returns the structured run output, or a
    typed error. The run is paced in real time by a monotonic steady clock.
    */
    [[nodiscard]] std::expected<HilRunOutput, HilError> run(std::span<const sim::sil::FaultScenario> scenarios);

private:
    [[nodiscard]] static std::expected<std::unique_ptr<HilRunContext>, HilError> makeContext(const HilConfig& config,
                                                                                                std::span<const sim::sil::FaultScenario> scenarios);
    static void execute(HilRunContext& ctx);
    static void finalize(HilRunContext& ctx);

    HilConfig     config_;
    std::ostream* live_out_ = nullptr;
};

}

namespace sim::hil {

/*
Internal helpers shared across the HilRunner-*.cpp translation units (module-internal,
not exported to importers). They keep the loop, actuator exchange/application, metrics
and deadline handling factored into focused units.
*/
void apply_injectors(HilRunContext& ctx);
void exchange_actuators(HilRunContext& ctx);
void update_health_and_safety(HilRunContext& ctx);
void apply_actuators(HilRunContext& ctx);
void update_metrics(HilRunContext& ctx);
void handle_deadline(HilRunContext& ctx, const HilStepTiming& timing);

}
