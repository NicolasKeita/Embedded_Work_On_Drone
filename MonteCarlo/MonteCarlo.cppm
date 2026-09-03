/*
Filename: MonteCarlo/MonteCarlo.cppm
Description: Public interface of the generic Monte-Carlo engine (pi estimation and 1D integration).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module MonteCarlo;

import std;

export struct MonteCarloResult
{
    std::float64_t estimate       = 0.0;
    std::float64_t standard_error = 0.0;
    std::uint64_t  samples        = 0;
};

/*
Plain function-pointer signature sampled by the integrator: deterministic call
overhead and no heap allocation, unlike std::function.
*/
export using MonteCarloFunction = std::float64_t (*)(std::float64_t);

export class MonteCarloEngine
{
public:
    explicit MonteCarloEngine(std::uint64_t seed);

    [[nodiscard]] MonteCarloResult estimate_pi(std::uint64_t samples);
    [[nodiscard]] MonteCarloResult integrate(
        MonteCarloFunction func,
        std::float64_t lower_bound, std::float64_t upper_bound, std::uint64_t samples);

private:
    std::mt19937_64 generator_;
};