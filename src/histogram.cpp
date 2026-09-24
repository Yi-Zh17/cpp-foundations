#include <foundations/histogram.hpp>

#include <bit>

uint64_t now_ns() {
    // Record current time
    const auto start = std::chrono::steady_clock::now();
    const auto start_duration = start.time_since_epoch();

    // Cast to nanoseconds
    const auto dur_nano = std::chrono::duration_cast<std::chrono::nanoseconds> (start_duration);

    return static_cast<uint64_t> (dur_nano.count());
}

Histogram::Histogram() {
    
}

void Histogram::record(uint64_t ns) {
    // Calculate bucket index
    const auto index = std::bit_width(ns) - 1;

    counts[static_cast<size_t>(index)]++;
}

uint64_t Histogram::get_count(unsigned long index) {
    return counts.at(index);
}
