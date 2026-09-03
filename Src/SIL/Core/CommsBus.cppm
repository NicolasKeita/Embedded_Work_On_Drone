/*
Filename: Src/SIL/Core/CommsBus.cppm
Description: Simulated FC1-to-FC2 communication bus with link cutoff and packet loss.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module CommsBus;

import std;

export namespace sim::sil {

// Observational delivery outcome of one published message (no behavioral effect).
struct CommsDelivery {
    std::uint64_t sequence = 0;
    double        send_time = 0.0;
    bool          delivered = false;
    double        receive_time = -1.0;
    double        latency_s = 0.0;
};

// Aggregate FC1-to-FC2 communication statistics collected by the SIL engine.
struct CommsStats {
    std::uint64_t sent = 0;
    std::uint64_t delivered = 0;
    std::uint64_t dropped = 0;
    std::uint64_t duplicated = 0;
    std::uint64_t reordered = 0;
    std::uint64_t timeouts = 0;
    std::uint64_t last_sequence = 0;
    double        latency_min_s = -1.0;
    double        latency_max_s = -1.0;
    double        latency_mean_s = -1.0;

    void record(const CommsDelivery& delivery);

    void record_timeout() noexcept;
};

class CommsBus {
public:
    explicit CommsBus(std::uint64_t seed = 42);

    void seed(std::uint64_t seed);

    void set_link(bool up, double loss_probability);

    void set_transport_latency(double latency_s) noexcept;

    // Publishes one sequenced message; returns its full delivery outcome.
    [[nodiscard]] CommsDelivery publish(double time);

    [[nodiscard]] bool link_up() const noexcept;

    [[nodiscard]] double last_received_time() const noexcept;

private:
    std::mt19937_64 generator_;
    bool            link_up_ = true;
    double          loss_probability_ = 0.0;
    double          transport_latency_s_ = 0.0;
    double          last_received_time_ = -1.0;
    std::uint64_t   sequence_ = 0;
};

}
