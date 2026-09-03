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
    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);

    std::uint64_t hits = 0;
    for (std::uint64_t i = 0; i < samples; ++i) {
        const std::float64_t x = unit(generator_);
        const std::float64_t y = unit(generator_);
        if (x * x + y * y <= 1.0) {
            ++hits;
        }
    }

    const std::float64_t ratio = static_cast<std::float64_t>(hits) / static_cast<std::float64_t>(samples);
    const std::float64_t variance = ratio * (1.0 - ratio) / static_cast<std::float64_t>(samples);

    return { 4.0 * ratio, 4.0 * std::sqrt(variance), samples };
}

/*
Integrates func over [lower_bound, upper_bound] by the mean of uniform samples:
I = (b - a) * E[f(u)], the standard error being (b - a) * empirical standard
deviation / sqrt(n). Aggregation streams through Welford's online variance so
that no dynamic allocation occurs regardless of the sample count.
*/
MonteCarloResult MonteCarloEngine::integrate(
    MonteCarloFunction func,
    std::float64_t lower_bound, std::float64_t upper_bound, std::uint64_t samples)
{
    const std::float64_t width = upper_bound - lower_bound;
    std::uniform_real_distribution<std::float64_t> domain(lower_bound, upper_bound);

    std::uint64_t count = 0;
    std::float64_t mean = 0.0;
    std::float64_t squaredDeviationSum = 0.0;
    for (std::uint64_t i = 0; i < samples; ++i) {
        const std::float64_t value = func(domain(generator_));
        ++count;
        const std::float64_t delta = value - mean;
        mean += delta / static_cast<std::float64_t>(count);
        squaredDeviationSum += delta * (value - mean);
    }

    const std::float64_t deviation =
        std::sqrt(squaredDeviationSum / static_cast<std::float64_t>(count));
    const std::float64_t standardError =
        width * deviation / std::sqrt(static_cast<std::float64_t>(count));

    return { width * mean, standardError, samples };
}