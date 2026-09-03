/*
Filename: MonteCarlo/main.cpp
Description: Demonstration entry point running Monte-Carlo experiments on pi and integrals.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import MonteCarlo;

void PrintResult(const std::string& label, const MonteCarloResult& result);

int main()
{
    constexpr std::array kSampleCounts{ std::uint64_t{ 1000 }, std::uint64_t{ 100000 }, std::uint64_t{ 10000000 } };
    constexpr std::float64_t kPiExact = 3.14159265358979323846;

    MonteCarloEngine engine(20260825u);

    std::cout << "=== Estimation de pi par echantillonnage uniforme ===" << std::endl;
    for (const std::uint64_t samples : kSampleCounts) {
        const MonteCarloResult result = engine.estimate_pi(samples);
        PrintResult("pi", result);
        std::cout << "    |erreur| relative : "
                  << std::scientific << std::setprecision(3)
                  << std::abs(result.estimate - kPiExact) / kPiExact << std::endl;
    }

    std::cout << "\n=== Integrale de f(x) = sin(x) sur [0, pi] ===" << std::endl;
    const auto sine = [](std::float64_t x) { return std::sin(x); };

    for (const std::uint64_t samples : kSampleCounts) {
        const MonteCarloResult result =
            engine.integrate(sine, 0.0, kPiExact, samples);
        PrintResult("I", result);
        std::cout << "    |erreur| absolue   : "
                  << std::scientific << std::setprecision(3)
                  << std::abs(result.estimate - 2.0) << std::endl;
    }

    return 0;
}

/*
Affiche une estimation avec son erreur-type et l'intervalle de confiance a 95%.
*/
void PrintResult(const std::string& label, const MonteCarloResult& result)
{
    std::cout << "\n  " << std::setw(2) << label
              << " | " << std::fixed << std::setprecision(0)
              << result.samples << " tirages"
              << " -> estimation = "
              << std::setprecision(5) << result.estimate
              << ", erreur-type = " << result.standard_error
              << ", IC 95% = [" << result.estimate - 1.96 * result.standard_error
              << " ; " << result.estimate + 1.96 * result.standard_error
              << "]" << std::endl;
}