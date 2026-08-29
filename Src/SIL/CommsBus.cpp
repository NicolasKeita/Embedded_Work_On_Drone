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
Livraison d'un message : echec si la liaison physique est coupee ou si le tirage
aleatoire (seede, pour la reproductibilite Monte Carlo) tombe sous le taux de
perte configure.
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
