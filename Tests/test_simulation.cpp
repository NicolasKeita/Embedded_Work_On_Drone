/*
Filename: Tests/test_simulation.cpp
Description: Deterministic validation scenarios (A to F) for the aircraft physics simulator at 100 Hz.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import Aircraft;

namespace
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kDt = 0.01; // Boucle a 100 Hz.
    constexpr int kLogLevelEverySteps = 100; // Un log toutes les 1 s.

    int g_failureCount = 0;

    void Check(bool condition, const std::string& label)
    {
        if (condition) {
            std::cout << "  [PASS] " << label << std::endl;
        }
        else {
            ++g_failureCount;
            std::cout << "  [FAIL] " << label << std::endl;
        }
    }

    void LogHeader()
    {
        std::cout << "      t(s)";
        std::cout << std::setw(11) << "x(m)" << std::setw(11) << "y(m)"
                  << std::setw(11) << "z(m)" << std::setw(11) << "vx(m/s)"
                  << std::setw(11) << "vy(m/s)" << std::setw(11) << "vz(m/s)"
                  << std::setw(11) << "pitch(d)" << std::setw(11) << "roll(d)"
                  << std::setw(11) << "rpm" << std::endl;
    }

    void LogStep(double timeSeconds, const Aircraft& aircraft)
    {
        const AircraftState& s = aircraft.state();

        std::cout << std::fixed << std::setw(9) << std::setprecision(2) << timeSeconds
                  << std::setw(11) << std::setprecision(3) << s.x
                  << std::setw(11) << s.y
                  << std::setw(11) << s.z
                  << std::setw(11) << s.vx
                  << std::setw(11) << s.vy
                  << std::setw(11) << s.vz
                  << std::setw(11) << std::setprecision(2) << s.pitch * 180.0 / kPi
                  << std::setw(11) << s.roll * 180.0 / kPi
                  << std::setw(11) << std::setprecision(0) << s.actual_rpm
                  << std::defaultfloat << std::endl;
    }

    // Avance la simulation de durationSecondes en loggant periodiquement l'etat.
    void Run(Aircraft& aircraft, double startTimeSeconds, double durationSeconds)
    {
        const int steps = static_cast<int>(durationSeconds / kDt + 0.5);

        for (int i = 0; i < steps; ++i) {
            aircraft.update(kDt);

            if (i % kLogLevelEverySteps == 0) {
                LogStep(startTimeSeconds + i * kDt, aircraft);
            }
        }

        LogStep(startTimeSeconds + durationSeconds, aircraft);
    }

    // Phase commune aux scenarios aeriens : montee rapide pour prendre de l'altitude.
    // Retourne l'instant de fin de la phase.
    double TakeOff(Aircraft& aircraft, double hoverRpm)
    {
        aircraft.set_command({1.3 * hoverRpm, 0.0, 0.0});
        Run(aircraft, 0.0, 3.0);
        return 3.0;
    }

    // Scenarios de validation -------------------------------------------------

    void ScenarioRest(double /*hoverRpm*/)
    {
        std::cout << "\n=== Scenario A : repos (RPM = 0, servos = 0) ===" << std::endl;
        LogHeader();

        Aircraft aircraft;
        Run(aircraft, 0.0, 3.0);

        const AircraftState& s = aircraft.state();
        Check(s.z == 0.0, "A1 : l'appareil reste pose au sol (z = 0)");
        Check(s.vx == 0.0 && s.vy == 0.0 && s.vz == 0.0, "A2 : vitesses nulles");
        Check(s.pitch == 0.0 && s.roll == 0.0, "A3 : attitude neutre");
        Check(s.actual_rpm == 0.0, "A4 : RPM effectif nul");
    }

    void ScenarioClimb(double hoverRpm)
    {
        std::cout << "\n=== Scenario B : montee (RPM = 1.1 x hover, servos = 0) ===" << std::endl;
        LogHeader();

        Aircraft aircraft;
        aircraft.set_command({1.1 * hoverRpm, 0.0, 0.0});
        Run(aircraft, 0.0, 6.0);

        const AircraftState& s = aircraft.state();
        Check(s.z > 1.0, "B1 : altitude en croissance (z > 1 m)");
        Check(s.vz > 0.0, "B2 : vitesse verticale positive");
        Check(s.actual_rpm > hoverRpm, "B3 : RPM effectif superieur au stationnaire");
    }

    void ScenarioDescent(double hoverRpm)
    {
        std::cout << "\n=== Scenario C : descente (montee puis RPM = 0.6 x hover) ===" << std::endl;
        LogHeader();

        Aircraft aircraft;
        const double takeoffEnd = TakeOff(aircraft, hoverRpm);
        const double topAltitude = aircraft.state().z;

        aircraft.set_command({0.6 * hoverRpm, 0.0, 0.0});
        Run(aircraft, takeoffEnd, 10.0);

        const AircraftState& s = aircraft.state();
        Check(s.vz <= 0.0, "C1 : vitesse verticale negative en fin de phase");
        Check(s.z < topAltitude, "C2 : altitude inferieure au sommet atteint");
        Check(s.z == 0.0, "C3 : retour au sol (blocage a z = 0)");
    }

    void ScenarioMoveX(double hoverRpm)
    {
        std::cout << "\n=== Scenario D : deplacement X (hover + pitch > 0) ===" << std::endl;
        LogHeader();

        Aircraft aircraft;
        const double takeoffEnd = TakeOff(aircraft, hoverRpm);

        aircraft.set_command({hoverRpm, 10.0, 10.0}); // Consigne moyenne positive -> pitch > 0.
        Run(aircraft, takeoffEnd, 6.0);

        const AircraftState& s = aircraft.state();
        Check(s.pitch > 0.0, "D1 : tangage positif");
        Check(s.vx > 0.0, "D2 : vitesse X positive");
        Check(s.x > 1.0, "D3 : deplacement vers les X positifs (x > 1 m)");
        Check(s.y == 0.0 && s.vy == 0.0, "D4 : pas de derivation laterale");
    }

    void ScenarioMoveY(double hoverRpm)
    {
        std::cout << "\n=== Scenario E : deplacement Y (hover + roll > 0) ===" << std::endl;
        LogHeader();

        Aircraft aircraft;
        const double takeoffEnd = TakeOff(aircraft, hoverRpm);

        aircraft.set_command({hoverRpm, 12.0, -12.0}); // Servos opposes : differentiel pur -> roll > 0 sans pitch.
        Run(aircraft, takeoffEnd, 6.0);

        const AircraftState& s = aircraft.state();
        Check(s.roll > 0.0, "E1 : roulis positif");
        Check(s.vy > 0.0, "E2 : vitesse Y positive");
        Check(s.y > 1.0, "E3 : deplacement vers les Y positifs (y > 1 m)");
        Check(s.x == 0.0 && s.vx == 0.0, "E4 : pas de derivation longitudinale");
    }

    void ScenarioCombined(double hoverRpm)
    {
        std::cout << "\n=== Scenario F : combine (RPM > hover, pitch > 0, roll < 0) ===" << std::endl;
        LogHeader();

        Aircraft aircraft;
        const double takeoffEnd = TakeOff(aircraft, hoverRpm);

        aircraft.set_command({1.15 * hoverRpm, -5.0, 15.0}); // Moyenne > 0, differentiel < 0.
        Run(aircraft, takeoffEnd, 6.0);

        const AircraftState& s = aircraft.state();
        Check(s.pitch > 0.0 && s.roll < 0.0, "F1 : attitude combinee (pitch > 0, roll < 0)");
        Check(s.vz > 0.0, "F2 : montee (vz > 0)");
        Check(s.vx > 0.0 && s.x > 1.0, "F3 : deplacement X positif");
        Check(s.vy < 0.0 && s.y < 0.0, "F4 : deplacement Y negatif");
    }

    // Registre des scénarios lancables individuellement -----------------------

    struct ScenarioEntry
    {
        char key;                // Lettre de sélection en ligne de commande.
        const char* description; // Rappel du scénario.
        void (*run)(double);     // Pointeur vers le scénario (hoverRpm en paramètre).
    };

    constexpr std::array<ScenarioEntry, 6> kScenarios{{
        {'a', "Repos (RPM = 0, servos = 0)", ScenarioRest},
        {'b', "Montee (RPM > hover)", ScenarioClimb},
        {'c', "Descente (RPM < hover)", ScenarioDescent},
        {'d', "Deplacement X (hover + pitch > 0)", ScenarioMoveX},
        {'e', "Deplacement Y (hover + roll > 0)", ScenarioMoveY},
        {'f', "Combine (RPM > hover, pitch > 0, roll < 0)", ScenarioCombined},
    }};

    void PrintUsage(const char* executableName)
    {
        std::cout << "Validation du simulateur physique (Heliblade-like)." << std::endl;
        std::cout << "Utilisation : " << executableName << " [scenario ...]" << std::endl;
        std::cout << "  Sans argument : tous les scenarios sont executes." << std::endl;
        std::cout << "  scenario      : lettre(s) parmi";
        for (const ScenarioEntry& entry : kScenarios) {
            std::cout << ' ' << entry.key;
        }
        std::cout << " (insensible a la casse), ou -h / --help." << std::endl;
        std::cout << std::endl;
        std::cout << "Scenarios disponibles :" << std::endl;

        for (const ScenarioEntry& entry : kScenarios) {
            std::cout << "  " << entry.key << " : " << entry.description << std::endl;
        }
    }

    // Resout un argument de ligne de commande en scenario ; nullptr si inconnu.
    const ScenarioEntry* FindScenario(char argument)
    {
        char normalizedKey = argument;

        // Normalisation manuelle de la casse (pas de dependance a <cctype>).
        if (normalizedKey >= 'A' && normalizedKey <= 'Z') {
            normalizedKey = static_cast<char>(normalizedKey + ('a' - 'A'));
        }

        for (const ScenarioEntry& entry : kScenarios) {
            if (entry.key == normalizedKey) {
                return &entry;
            }
        }

        return nullptr;
    }
}

int main(int argc, char* argv[])
{
    const char* executableName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "test_simulation";

    // Selection des scenarios depuis la ligne de commande.
    std::vector<const ScenarioEntry*> selected;

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i] != nullptr ? argv[i] : "";

        if (argument == "-h" || argument == "--help") {
            PrintUsage(executableName);
            return 0;
        }

        if (argument.size() != 1 || FindScenario(argument[0]) == nullptr) {
            std::cout << "Erreur : argument invalide \"" << argument << "\"." << std::endl;
            std::cout << std::endl;
            PrintUsage(executableName);
            return 2;
        }

        selected.push_back(FindScenario(argument[0]));
    }

    // Sans argument : tous les scenarios sont executes.
    if (selected.empty()) {
        for (const ScenarioEntry& entry : kScenarios) {
            selected.push_back(&entry);
        }
    }

    const Aircraft reference;
    const double hoverRpm = reference.hover_rpm();

    std::cout << "=== Validation simulateur physique (Heliblade-like) ===" << std::endl;
    std::cout << "RPM de stationnaire theorique : "
              << std::fixed << std::setprecision(1) << hoverRpm << " tr/min" << std::endl;
    std::cout << "Boucle mono-thread deterministic a 100 Hz (dt = 0.01 s)." << std::endl;

    for (const ScenarioEntry* entry : selected) {
        entry->run(hoverRpm);
    }

    if (g_failureCount == 0) {
        std::cout << "\n>>> Tous les scenarios lances sont valides (PASS)." << std::endl;
        return 0;
    }

    std::cout << "\n>>> " << g_failureCount << " verification(s) ont echoue (FAIL)." << std::endl;
    return 1;
}