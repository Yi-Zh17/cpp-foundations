#include <foundations/custom_clock.hpp>

uint64_t now_ns() {
    // Record current time
    const auto start = std::chrono::steady_clock::now();
    const auto start_duration = start.time_since_epoch();

    // Cast to nanoseconds
    const auto dur_nano = std::chrono::duration_cast<std::chrono::nanoseconds> (start_duration);

    return static_cast<uint64_t> (dur_nano.count());
}