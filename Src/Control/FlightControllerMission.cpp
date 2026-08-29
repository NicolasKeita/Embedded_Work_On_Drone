/*
Filename: Src/Control/FlightControllerMission.cpp
Description: Mission state machine of the autonomous flight controller (TAKEOFF to COMPLETE).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightController;

import std;

import Aircraft;

namespace sim::control {

namespace
{
    double Clamp(double value, double minValue, double maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

/*
Machine a etats de mission :
  TAKEOFF         armement a RPM fixe (takeoff_rpm_factor x hover_rpm), puis
                  bascule en CLIMB des que takeoff_altitude_m est atteinte ;
  CLIMB           montee pilotee par la boucle d'altitude vers target.z, puis
                  STATION_KEEPING des que la tolerance altitude est tenue ;
  STATION_KEEPING asservissement simultane X/Y/Z ; COMPLETE apres
                  station_hold_seconds consecutives dans la zone cible ;
  COMPLETE        la tenue de station continue pour maintenir la position.
*/
ControlCommand FlightController::update(const TargetState&   target,
                                        const AircraftState& actual,
                                        double               dt)
{
    switch (mission_state_) {
    case MissionState::TAKEOFF: {
        ControlCommand cmd;
        cmd.wing_rpm = Clamp(config_.takeoff_rpm_factor * config_.hover_rpm,
                             config_.min_rpm, config_.max_rpm);

        if (actual.z >= config_.takeoff_altitude_m) {
            enter_climb();
        }
        return cmd;
    }
    case MissionState::CLIMB: {
        ControlCommand cmd;
        cmd.wing_rpm = updateAltitudeControl(target.z, actual.z, dt);

        if (std::abs(target.z - actual.z) <= config_.altitude_tolerance_m) {
            reset_pids();
            station_hold_timer_ = 0.0;
            mission_state_ = MissionState::STATION_KEEPING;
        }
        return cmd;
    }
    case MissionState::STATION_KEEPING: {
        ControlCommand cmd = station_keeping_command(target, actual, dt);

        if (inside_target_zone(target, actual)) {
            station_hold_timer_ += dt;
            if (station_hold_timer_ >= config_.station_hold_seconds) {
                mission_state_ = MissionState::COMPLETE;
            }
        }
        else {
            station_hold_timer_ = 0.0;
        }
        return cmd;
    }
    case MissionState::COMPLETE:
        return station_keeping_command(target, actual, dt);
    }

    return ControlCommand{};
}

std::string_view mission_state_name(MissionState state)
{
    switch (state) {
    case MissionState::TAKEOFF:
        return "TAKEOFF";
    case MissionState::CLIMB:
        return "CLIMB";
    case MissionState::STATION_KEEPING:
        return "STATION_KEEPING";
    case MissionState::COMPLETE:
        return "COMPLETE";
    }
    return "UNKNOWN";
}

}
