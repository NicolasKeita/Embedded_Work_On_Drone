/*
Filename: Src/SIL/Core/CommsBus.cpp
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

void CommsBus::set_link(bool up, std::float64_t loss_probability)
{
    link_up_ = up;
    loss_probability_ = loss_probability;
}

void CommsBus::set_transport_latency(std::float64_t latency_s) noexcept
{
    transport_latency_s_ = std::max(std::float64_t{0.0}, latency_s);
}

/*
Aggregate statistics: counts one delivery outcome and updates the running
latency mean incrementally (no stored sample buffer).
*/
void CommsStats::record(const CommsDelivery& delivery)
{
    ++sent;
    last_sequence = delivery.sequence;
    if (!delivery.delivered) {
        ++dropped;
        return;
    }
    ++delivered;
    if (latency_min_s < 0.0 || delivery.latency_s < latency_min_s) {
        latency_min_s = delivery.latency_s;
    }
    latency_max_s = std::max(latency_max_s, delivery.latency_s);
    if (latency_mean_s < 0.0) {
        latency_mean_s = delivery.latency_s;
        return;
    }
    latency_mean_s += (delivery.latency_s - latency_mean_s) / static_cast<std::float64_t>(delivered);
}

void CommsStats::record_timeout() noexcept
{
    ++timeouts;
}

/*
Message delivery: fails if the physical link is down or if the random draw
(seeded, for Monte Carlo reproducibility) falls below the configured loss rate.
The receive timestamp carries the simulated transport latency as an
observational attribute; FC2-side detection keeps using the send tick, so the
delivery semantics are unchanged.
*/
CommsDelivery CommsBus::publish(std::float64_t time)
{
    CommsDelivery delivery;

    delivery.sequence = ++sequence_;
    delivery.send_time = time;
    delivery.latency_s = transport_latency_s_;

    if (!link_up_) {
        return delivery;
    }
    if (loss_probability_ > 0.0) {
        const std::float64_t draw = std::uniform_real_distribution<std::float64_t>{0.0, 1.0}(generator_);
        if (draw < loss_probability_) {
            return delivery;
        }
    }
    delivery.delivered = true;
    delivery.receive_time = time + transport_latency_s_;
    last_received_time_ = time;
    return delivery;
}

bool CommsBus::link_up() const noexcept
{
    return link_up_;
}

std::float64_t CommsBus::last_received_time() const noexcept
{
    return last_received_time_;
}

}
