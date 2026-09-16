/*
Filename: Src/Embedded/Status/StatusLed.cppm
Description: Nonblocking firmware activity and fault indicator interface.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module StatusLed;

import std;

export namespace FlightCore::Status {

class StatusLed {
public:
    /* Configures the board user LED; returns the Zephyr error on failure. */
    [[nodiscard]] std::expected<void, std::int32_t> initialize() noexcept;

    /* Records completed application work using monotonic milliseconds. */
    void mark_activity(std::int64_t now_ms) noexcept;

    /* Advances the indicator from the application loop without sleeping or allocating. */
    [[nodiscard]] std::expected<void, std::int32_t> update(std::int64_t now_ms, bool fault) noexcept;

private:
    std::int64_t last_activity_ms_ = -1;
    std::int64_t pattern_started_ms_ = 0;
    std::uint8_t mode_ = 0;
    bool initialized_ = false;
    bool illuminated_ = false;
};

}
