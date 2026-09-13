/*
Filename: Src/Embedded/Hil/Runner/Safety/HilRunnerSafety.cppm
Description: Safety interface of the HIL runner : embedded-FC2 supervision selection,
health evaluation dispatch (physical FC2 report or host-side HealthMonitor) and
application of the resulting SAFE_MODE/COMPENSATED actuator overrides to the aircraft.
Exports:
    uses_embedded_fc2_supervision(),
    update_health_and_safety(),
    apply_actuators()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilRunnerSafety;

import std;

import HealthMonitor;
import HilRunnerContext;

export namespace sim::hil {

/* Selects physical FC2 supervision whenever the HIL target is a serial STM32. */
[[nodiscard]] bool uses_embedded_fc2_supervision(const HilRunContext& ctx) noexcept;

/* Evaluates host or embedded FC2 health and records the resulting safety command. */
void update_health_and_safety(HilRunContext& ctx);

/* Applies the safety mode to the actuator command and integrates the aircraft physics. */
void apply_actuators(HilRunContext& ctx);

}