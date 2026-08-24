/*
Filename: Tests/test_simulation.cpp
Description: Entry point running the deterministic validation scenarios at 100 Hz.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import Aircraft;
import ScenarioCatalog;
import TestHarness;

namespace
{
    // Resout la selection de scenarios ; retourne un code >= 0 pour terminer immediatement.
    int SelectScenarios(int argc, char* argv[], std::string_view executableName,
                        std::vector<const sim::test::ScenarioEntry*>& selected)
    {
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i] != nullptr ? argv[i] : "";

            if (argument == "-h" || argument == "--help") {
                sim::test::ScenarioCatalog::print_usage(executableName);
                return 0;
            }

            if (argument.size() != 1 || sim::test::ScenarioCatalog::find(argument[0]) == nullptr) {
                std::cout << "Erreur : argument invalide \"" << argument << "\"." << std::endl;
                std::cout << std::endl;
                sim::test::ScenarioCatalog::print_usage(executableName);
                return 2;
            }

            selected.push_back(sim::test::ScenarioCatalog::find(argument[0]));
        }

        if (selected.empty()) {
            for (const sim::test::ScenarioEntry& entry : sim::test::ScenarioCatalog::all()) {
                selected.push_back(&entry);
            }
        }

        return -1;
    }
}

/*
Point d'entree : un seul runner est partage par tous les scenarios afin que le
compteur d'echecs soit cumule sur l'ensemble de la validation.
*/
int main(int argc, char* argv[])
{
    const std::string executableName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "test_simulation";

    std::vector<const sim::test::ScenarioEntry*> selected;
    const int earlyStatus = SelectScenarios(argc, argv, executableName, selected);
    if (earlyStatus >= 0) {
        return earlyStatus;
    }

    const Aircraft reference;
    const double hoverRpm = reference.hover_rpm();

    sim::test::TestHarness runner{sim::test::HarnessConfig{.dt = 0.01, .log_interval_steps = 100}};

    std::cout << "=== Validation simulateur physique (Heliblade-like) ===" << std::endl;
    std::cout << "RPM de stationnaire theorique : "
              << std::fixed << std::setprecision(1) << hoverRpm << " tr/min" << std::endl;
    std::cout << "Boucle mono-thread deterministic a 100 Hz (dt = 0.01 s)." << std::endl;

    for (const sim::test::ScenarioEntry* entry : selected) {
        entry->run(runner, hoverRpm);
    }

    if (runner.passed()) {
        std::cout << "\n>>> Tous les scenarios lances sont valides (PASS)." << std::endl;
        return 0;
    }

    std::cout << "\n>>> " << runner.failure_count()
              << " verification(s) ont echoue (FAIL)." << std::endl;
    return 1;
}
