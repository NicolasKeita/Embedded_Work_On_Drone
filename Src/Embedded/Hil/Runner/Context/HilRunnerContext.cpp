/*
Filename: Src/Embedded/Hil/Runner/Context/HilRunnerContext.cpp
Description: Construction of the HIL run context (wiring the reused SIL core with the HIL
sensor model, transport and clock defaults) and the safety-behaviour verdict.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerContext;

import std;

import Aircraft;
import CommsBus;
import FaultInjectors;
import FlightController;
import FlightControllerTypes;
import HalTypes;
import HealthMonitor;
import HilClock;
import HilConfig;
import HilEvents;
import HilFcTarget;
import HilProtocol;
import HilSensorModel;
import HilTelemetry;
import HilTiming;
import HilTransport;
import SafetyManager;
import SilFaultScenario;
import SilTypes;
import Telemetry;
import Transport;

namespace sim::hil {

HilRunContext::HilRunContext(const HilConfig& cfg,
                             std::unique_ptr<FlightCore::Transport::ITransport> byte_channel)
    : config{cfg},
      aircraft{},
      comms{cfg.seed},
      health{sim::safety::HealthMonitorConfig{
          .heartbeat_timeout_s = cfg.heartbeat_timeout_s,
          .actuator_mismatch_rpm = cfg.actuator_mismatch_rpm,
          .actuator_mismatch_hold_s = cfg.actuator_mismatch_hold_s,
          .sensor_limits = cfg.sensor_limits,
      }},
      safety{sim::safety::SafetyManagerConfig{.degraded_thrust_margin = cfg.thrust_compensation_margin}},
      safety_command{},
      sensor_model{cfg.sensor_noise_stddev, cfg.seed},
      channel{std::move(byte_channel)},
      transport{channel.get()},
      fc_target{},
      clock{},
      injectors{},
      injector_count{0},
      env{},
      sensors{},
      sampled_truth{},
      last_sensor_data{},
      actuator_cmd{},
      actuator_diagnostics{},
      command{},
      result{},
      trace{},
      telemetry_recorder{},
      timing{},
      previous_mission_state{sim::control::MissionState::SPIN_UP},
      previous_safety_mode{sim::safety::SafetyMode::NORMAL},
      previous_health{sim::safety::HealthState::HEALTHY},
      fc1_was_alive{true}
{
    telemetry_recorder.interval_s = (cfg.telemetry_rate_hz > 0.0) ? 1.0 / cfg.telemetry_rate_hz : 0.05;
    result.scenario_id = cfg.scenario_id;
}

}
