/*
Filename: MonteCarlo/MonteCarlo.cpp
Description: Implementation of the Monte-Carlo estimators (uniform pi sampling and mean-based 1D integration).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MonteCarlo;

import std;

/*
Construit le moteur avec une graine deterministe pour des executions reproductibles.
*/
MonteCarloEngine::MonteCarloEngine(std::uint64_t seed)
    : generator_(seed)
{
}

/*
Echantillonne des points uniformes dans le carre unite et compte la fraction
tombant dans le quart de disque de rayon 1 : l'estimateur vaut 4 * p, avec une
erreur-type binomiale 4 * sqrt(p * (1 - p) / n).
*/
MonteCarloResult MonteCarloEngine::estimate_pi(std::uint64_t samples)
{
    std::uniform_real_distribution<double> unit(0.0, 1.0);

    std::uint64_t hits = 0;
    for (std::uint64_t i = 0; i < samples; ++i) {
        const double x = unit(generator_);
        const double y = unit(generator_);
        if (x * x + y * y <= 1.0) {
            ++hits;
        }
    }

    const double ratio = static_cast<double>(hits) / static_cast<double>(samples);
    const double variance = ratio * (1.0 - ratio) / static_cast<double>(samples);

    return { 4.0 * ratio, 4.0 * std::sqrt(variance), samples };
}

/*
Integre func sur [lower_bound, upper_bound] par moyenne d'echantillons uniformes :
I = (b - a) * E[f(u)], l'erreur-type etant (b - a) * ecart-type empirique / sqrt(n).
*/
MonteCarloResult MonteCarloEngine::integrate(
    const std::function<double(double)>& func,
    double lower_bound, double upper_bound, std::uint64_t samples)
{
    const double width = upper_bound - lower_bound;
    std::uniform_real_distribution<double> domain(lower_bound, upper_bound);

    std::vector<double> values(static_cast<std::size_t>(samples));
    for (double& value : values) {
        value = func(domain(generator_));
    }

    const double mean =
        std::reduce(values.begin(), values.end(), 0.0)
        / static_cast<double>(samples);

    double squaredDeviationSum = 0.0;
    for (const double value : values) {
        squaredDeviationSum += (value - mean) * (value - mean);
    }
    const double deviation =
        std::sqrt(squaredDeviationSum / static_cast<double>(samples));

    const double standardError =
        width * deviation / std::sqrt(static_cast<double>(samples));

    return { width * mean, standardError, samples };
}