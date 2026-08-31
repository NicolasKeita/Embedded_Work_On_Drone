/*
Filename: Src/SIL/Core/Events/SilEvents.cppm
Description: Structured SIL events, telemetry sampling and leveled trace recorder.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilEvents;

import std;

import Aircraft;

export namespace sim::sil {

// Typed simulation events emitted by the SIL pipeline (observability contract).
enum class SilEventType {
    SimulationStart,
    SimulationEnd,
    FaultInjected,
    FaultDetected,
    FaultClassified,
    SafetyResponse,
    SafetyStateTransition,
    MissionStateTransition,
    WatchdogTimeout,
    WatchdogKick,
    WatchdogRecovery,
    FCStartup,
    FCShutdown,
    FCFailure,
    MessageGenerated,
    MessageDelivered,
    MessageDropped,
    MessageTimeout,
    HeartbeatSent,
    HeartbeatDelivered,
    HeartbeatDropped,
    SensorFault,
    ActuatorFault,
    RecoveryStart,
    RecoveryEnd
};

enum class EventSeverity { Info, Warning, Error, Debug, Trace };

/* One structured simulation event; string fields reference static literals. */
struct SilEvent {
    double timestamp = 0.0;
    std::string_view source{};
    SilEventType type = SilEventType::SimulationStart;
    EventSeverity severity = EventSeverity::Info;
    std::string_view detail{};
    std::string_view previous_state{};
    std::string_view new_state{};
    std::string_view reason{};
    std::uint64_t sequence = 0;
    bool has_sequence = false;
    double latency_s = -1.0;
    bool has_latency = false;
    double value = 0.0;
    bool has_value = false;
};

// One sampled flight/control telemetry record (structured, never a log line).
struct TelemetrySample {
    double time = 0.0;
    double x = 0.0;
    double y = 0.0;
    double altitude_m = 0.0;
    double pitch_rad = 0.0;
    double roll_rad = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;
    double commanded_rpm = 0.0;
    double actual_rpm = 0.0;
    double left_servo_deg = 0.0;
    double right_servo_deg = 0.0;
};

enum class SilLogLevel { Info, Debug, Trace };

struct SilTraceConfig {
    SilLogLevel level = SilLogLevel::Info;
    std::size_t max_events = 500000;
};
// Leveled in-memory trace recorder: observational only, filtered by verbosity.
class SilTrace {
public:
    SilTrace() = default;
    explicit SilTrace(SilTraceConfig config);

    void record(SilEvent event);

    [[nodiscard]] std::span<const SilEvent> events() const noexcept;

    [[nodiscard]] std::vector<SilEvent> take_events() noexcept;

private:
    SilTraceConfig config_{};
    std::vector<SilEvent> events_;
};
// Fixed-rate telemetry sampler (rate configured through interval_s).
struct TelemetryRecorder {
    double interval_s = 0.05;
    double last_sample_time = -1.0e12;
    std::vector<TelemetrySample> samples;

    void maybe_record(double time, const AircraftState& state, const ControlCommand& command,
                      double commanded_rpm);
};

[[nodiscard]] std::string_view event_type_name(SilEventType type);
[[nodiscard]] std::string_view event_severity_name(EventSeverity severity);
// Verbosity level an event belongs to (warnings/errors are always kept).
[[nodiscard]] SilLogLevel event_log_level(EventSeverity severity);

}