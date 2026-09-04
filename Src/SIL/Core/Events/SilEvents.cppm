/*
Filename: Src/SIL/Core/Events/SilEvents.cppm
Description: Structured SIL events and leveled trace recorder.
Exports:
    enum class SilEventType,
    enum class EventSeverity,
    enum class SilLogLevel,
    struct SilEvent,
    struct SilTraceConfig,
    class SilTrace,
    event_type_name(),
    event_severity_name(),
    event_log_level()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilEvents;

import std;

import Aircraft;
import SilFaultScenario;

export namespace sim::sil {

// Typed simulation events emitted by the SIL pipeline (observability contract).
enum class SilEventType {
    SimulationStart,
    SimulationEnd,
    FaultInjected,
    FaultCleared,
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
    std::float64_t   timestamp = 0.0;
    std::string_view source{};
    SilEventType     type = SilEventType::SimulationStart;
    EventSeverity    severity = EventSeverity::Info;
    std::string_view detail{};
    std::string_view previous_state{};
    std::string_view new_state{};
    std::string_view reason{};
    std::uint64_t    sequence = 0;
    bool             has_sequence = false;
    std::float64_t   latency_s = -1.0;
    bool             has_latency = false;
    std::float64_t   value = 0.0;
    bool             has_value = false;
    std::float64_t   duration_s = 0.0;
    bool             has_duration = false;
    std::string_view target{};
    std::string_view target_signal{};
    std::string_view physical_role{};
    std::string_view physical_category{};
    std::string_view physical_function{};
    std::string_view subtype{};
    std::string_view expected_behavior{};
    FaultProfile     profile = FaultProfile::Permanent;
    bool             has_profile = false;
    FaultValueKind   value_kind = FaultValueKind::None;
};

enum class SilLogLevel { Info, Debug, Trace };

struct SilTraceConfig {
    SilLogLevel level = SilLogLevel::Info;
    std::size_t max_events = 500000;
};
/*
Verbosity-filtered in-memory trace recorder: observational only.
*/
class SilTrace {
public:
    SilTrace() = default;
    explicit SilTrace(SilTraceConfig config);

    void record(SilEvent event);

    [[nodiscard]] std::span<const SilEvent> events() const noexcept;

    [[nodiscard]] std::vector<SilEvent> take_events() noexcept;

private:
    SilTraceConfig        config_{};
    std::vector<SilEvent> events_;
};

[[nodiscard]] std::string_view event_type_name(SilEventType type);
[[nodiscard]] std::string_view event_severity_name(EventSeverity severity);
// Verbosity level an event belongs to (warnings/errors are always kept).
[[nodiscard]] SilLogLevel event_log_level(EventSeverity severity);

}
