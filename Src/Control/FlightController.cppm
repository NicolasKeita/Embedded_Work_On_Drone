/*
Filename: Src/Control/FlightController.cppm
Description: Public interface of the autonomous flight controller : mission state machine and cascaded position, altitude and attitude loops.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FlightController;

import std;

import Aircraft;

export namespace sim::control {

struct TargetState {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

enum class MissionState {
    TAKEOFF,
    CLIMB,
    STATION_KEEPING,
    COMPLETE
};

struct ControllerConfig {
    double hover_rpm{0.0};
    double kp_altitude{5.0};
    double ki_altitude{0.05};
    double kd_altitude{24.0};
    double max_integral_rpm{60.0};
    double kp_position{1.0};
    double kd_position{5.1};
    double kp_attitude{0.8};
    double max_tilt_deg{15.0};
    double min_rpm{300.0};
    double max_rpm{12000.0};
    double takeoff_rpm_factor{1.3};
    double takeoff_altitude_m{2.0};
    double altitude_tolerance_m{0.5};
    double position_tolerance_m{1.0};
    double station_hold_seconds{5.0};
};

// Consignes d'inclinaison produites par la boucle de position (boucle externe).
struct TiltTargets {
    double pitch_deg = 0.0;
    double roll_deg = 0.0;
};

// Melange servo produit par la boucle d'attitude (boucle interne).
struct ServoMix {
    double left_deg = 0.0;
    double right_deg = 0.0;
};

class FlightController {
public:
    explicit FlightController(ControllerConfig config);

    [[nodiscard]] MissionState state() const;

    /*
    Avance la machine a etats de mission d'un pas de temps dt et retourne la
    commande actionneurs (RPM aile + angles servos gauche/droit) calculee par
    les boucles en cascade a partir de la consigne cible et de l'etat mesure.
    */
    ControlCommand update(const TargetState& target, const AircraftState& actual, double dt);

private:
    struct AxisPid {
        double kp = 0.0;
        double ki = 0.0;
        double kd = 0.0;
        double integral = 0.0;
        double integral_limit = 0.0;
        double integral_error_band = 0.0;
        double previous_error = 0.0;
        bool primed = false;
    };

    void enter_climb();
    void reset_pids();
    static double AxisPidStep(AxisPid& pid, double error, double dt);
    double updateAltitudeControl(double target_z, double actual_z, double dt);
    TiltTargets updatePositionControl(const TargetState& t, const AircraftState& a, double dt);
    ServoMix updateAttitudeControl(TiltTargets tilt, const AircraftState& s) const;
    ControlCommand station_keeping_command(const TargetState& t, const AircraftState& a,
                                           double dt);
    [[nodiscard]] bool inside_target_zone(const TargetState& t, const AircraftState& a) const;

    ControllerConfig config_;
    MissionState mission_state_{MissionState::TAKEOFF};
    AxisPid altitude_pid_{};
    AxisPid x_position_pid_{};
    AxisPid y_position_pid_{};
    double station_hold_timer_{0.0};
};

// Nom lisible d'un etat de mission pour la journalisation.
[[nodiscard]] std::string_view mission_state_name(MissionState state);

}
