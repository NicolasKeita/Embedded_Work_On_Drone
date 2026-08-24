/*
Filename: Tests/TestHarness.cpp
Description: Implementation of the shared validation harness (checks, logging, run loop).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module TestHarness;

import std;

namespace
{
    constexpr double kTakeOffDurationSeconds = 3.0;
}

namespace sim::test {

/*
Verifie une condition et journalise explicitement le succes ou l'echec ;
l'echec est compte dans l'etat d'instance du runner (failures_).
*/
void TestRunner::check(bool condition, std::string_view label)
{
    if (condition) {
        std::cout << "  [PASS] " << label << std::endl;
    }
    else {
        ++failures_;
        std::cout << "  [FAIL] " << label << std::endl;
    }
}

void TestRunner::log_header() const
{
    std::cout << "      t(s)";
    std::cout << std::setw(11) << "x(m)" << std::setw(11) << "y(m)"
              << std::setw(11) << "z(m)" << std::setw(11) << "vx(m/s)"
              << std::setw(11) << "vy(m/s)" << std::setw(11) << "vz(m/s)"
              << std::setw(11) << "pitch(d)" << std::setw(11) << "roll(d)"
              << std::setw(11) << "rpm" << std::endl;
}

void TestRunner::log_step(const Aircraft& aircraft) const
{
    const AircraftState& s = aircraft.state();

    std::cout << std::fixed << std::setw(9) << std::setprecision(2) << current_time_
              << std::setw(11) << std::setprecision(3) << s.x
              << std::setw(11) << s.y
              << std::setw(11) << s.z
              << std::setw(11) << s.vx
              << std::setw(11) << s.vy
              << std::setw(11) << s.vz
              << std::setw(11) << std::setprecision(2) << s.pitch * 180.0 / std::numbers::pi
              << std::setw(11) << s.roll * 180.0 / std::numbers::pi
              << std::setw(11) << std::setprecision(0) << s.actual_rpm
              << std::defaultfloat << std::endl;
}

/*
Avance la simulation de durationSecondes sur l'aeronef passe, en suivant le temps
simule et le nombre de pas dans l'etat d'instance, et en loggant periodiquement
l'etat au rythme defini par HarnessConfig::log_interval_steps.
*/
void TestRunner::run(Aircraft& aircraft, double duration_seconds)
{
    const int steps = static_cast<int>(duration_seconds / config_.dt + 0.5);

    // Un log final est emis uniquement si le dernier pas n'a pas deja ete logge
    // par la periodicite (sinon la derniere ligne du tableau serait dupliquee).
    bool last_step_logged = false;

    for (int i = 0; i < steps; ++i) {
        aircraft.update(config_.dt);
        ++step_count_;
        current_time_ += config_.dt;

        if (config_.log_interval_steps > 0 && step_count_ % config_.log_interval_steps == 0) {
            log_step(aircraft);
            last_step_logged = true;
        }
    }

    if (!last_step_logged) {
        log_step(aircraft);
    }
}

/*
Phase commune aux scenarios aeriens : montee rapide pour prendre de l'altitude.
Le temps simule est suivi en interne par le runner (current_time_), la phase
n'a donc plus besoin de retourner son instant de fin.
*/
void TestRunner::take_off(Aircraft& aircraft, double target_rpm)
{
    aircraft.set_command({1.3 * target_rpm, 0.0, 0.0});
    run(aircraft, kTakeOffDurationSeconds);
}

// Reinitialise completement l'etat du runner : compteur d'echecs, temps et pas simules.
void TestRunner::reset()
{
    failures_ = 0;
    current_time_ = 0.0;
    step_count_ = 0;
}

} // namespace sim::test