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
    double estimate = 0.0;
    double standard_error = 0.0;
    std::uint64_t samples = 0;
};

export class MonteCarloEngine
{
public:
    explicit MonteCarloEngine(std::uint64_t seed);

    [[nodiscard]] MonteCarloResult estimate_pi(std::uint64_t samples);
    [[nodiscard]] MonteCarloResult integrate(
        const std::function<double(double)>& func,
        double lower_bound, double upper_bound, std::uint64_t samples);

private:
    std::mt19937_64 generator_;
};