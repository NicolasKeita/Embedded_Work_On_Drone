/*
Filename: Src/SIL/Core/CommsBus.cppm
Description: Simulated FC1-to-FC2 communication bus with link cutoff and packet loss.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module CommsBus;

import std;

export namespace sim::sil {

class CommsBus {
public:
    explicit CommsBus(std::uint64_t seed = 42);

    void seed(std::uint64_t seed);

    void set_link(bool up, double loss_probability);

    // Attempts delivery of a timestamped message; returns true when received by FC2.
    [[nodiscard]] bool publish(double time);

    [[nodiscard]] bool link_up() const noexcept;

    [[nodiscard]] double last_received_time() const noexcept;

private:
    std::mt19937_64 generator_;
    bool link_up_ = true;
    double loss_probability_ = 0.0;
    double last_received_time_ = -1.0;
};

}
