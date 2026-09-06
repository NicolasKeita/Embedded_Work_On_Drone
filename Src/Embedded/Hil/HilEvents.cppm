/*
Filename: Src/Embedded/Hil/HilEvents.cppm
Description: Structured HIL events and in-memory trace recorder. Event names follow
the HIL runner observability contract (HIL_RUN_START, MISSION_STATE_TRANSITION,
FAULT_INJECTED/CLEARED/DETECTED, SAFETY_STATE_TRANSITION, HEARTBEAT_TIMEOUT,
FC1_FAILURE, DEADLINE_MISSED, HIL_STEP_ERROR, MISSION_COMPLETE, MISSION_ABORTED,
HIL_RUN_END). Normal control iterations are never logged as high-level events.
Exports:
    enum class HilEventType,
    enum class HilEventSeverity,
    struct HilEvent,
    class HilTrace,
    event_type_name(),
    event_severity_name(),
    event_category(),
    is_report_event()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilEvents;

import std;

export namespace sim::hil {

enum class HilEventType {
    HilRunStart,
    HilRunEnd,
    MissionStateTransition,
    MissionComplete,
    MissionAborted,
    FaultInjected,
    FaultCleared,
    FaultDetected,
    SafetyStateTransition,
    HeartbeatTimeout,
    Fc1Failure,
    DeadlineMissed,
    HilStepError,
    CommandSent,
    CommandDropped,
    CommandReceived
};

enum class HilEventSeverity { Info, Warning, Error };

/*
One structured HIL event. sim_time_s is simulation time; wall_us is the monotonic
host clock instant. String members reference static literals so the recorder
allocates no memory for messages.
*/
struct HilEvent {
    std::float64_t    sim_time_s = 0.0;
    std::uint64_t     wall_us = 0;
    std::string_view  source{};
    HilEventType      type = HilEventType::HilRunStart;
    HilEventSeverity  severity = HilEventSeverity::Info;
    std::string_view  detail{};
    std::string_view  previous_state{};
    std::string_view  new_state{};
    std::string_view  reason{};
    std::uint64_t     sequence = 0;
    bool              has_sequence = false;
    std::int64_t      duration_us = -1;
    bool              has_duration = false;
};

/*
Fixed-capacity in-memory trace recorder (observational only: events never alter
the run). Keeps a vector of events; takeEvents() moves them out for reporting.
*/
class HilTrace {
public:
    HilTrace() = default;

    void record(HilEvent event);

    [[nodiscard]] std::span<const HilEvent> events() const noexcept;

    [[nodiscard]] std::vector<HilEvent> takeEvents() noexcept;

private:
    std::vector<HilEvent> events_;
};

[[nodiscard]] std::string_view event_type_name(HilEventType type) noexcept;
[[nodiscard]] std::string_view event_severity_name(HilEventSeverity severity) noexcept;

/* Short category label used to prefix events in the human-readable report ([MISSION]/[FAULT]/[SAFETY]/[TIMING]). */
[[nodiscard]] std::string_view event_category(HilEventType type) noexcept;

/* True when an event belongs to the human-readable event timeline (excludes per-step command noise). */
[[nodiscard]] bool is_report_event(HilEventType type) noexcept;

}
