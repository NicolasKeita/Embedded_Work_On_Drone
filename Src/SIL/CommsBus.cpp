/*
Filename: Src/SIL/CommsBus.cpp
Description: Delivery logic of the simulated communication bus.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module CommsBus;

import std;

namespace sim::sil {

CommsBus::CommsBus(std::uint64_t seed) : generator_{seed} {}

void CommsBus::seed(std::uint64_t seed)
{
    generator_.seed(seed);
}

void CommsBus::set_link(bool up, double loss_probability)
{
    link_up_ = up;
    loss_probability_ = loss_probability;
}

/*
Message delivery: fails if the physical link is down or if the random draw
(seeded, for Monte Carlo reproducibility) falls below the configured loss rate.
*/
bool CommsBus::publish(double time)
{
    if (!link_up_) {
        return false;
    }
    if (loss_probability_ > 0.0) {
        const double draw = std::uniform_real_distribution<double>{0.0, 1.0}(generator_);
        if (draw < loss_probability_) {
            return false;
        }
    }
    last_received_time_ = time;
    return true;
}

bool CommsBus::link_up() const noexcept
{
    return link_up_;
}

double CommsBus::last_received_time() const noexcept
{
    return last_received_time_;
}

}
