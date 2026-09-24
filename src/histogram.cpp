#include <foundations/histogram.hpp>

#include <bit>
#include <iomanip>
#include <string>


#if defined(__x86_64__)
#include <x86intrin.h>
#elif defined(__aarch64__)
#include <cstdint>
#endif

uint64_t now_ns() {
    // Record current time
    const auto start = std::chrono::steady_clock::now();
    const auto start_duration = start.time_since_epoch();

    // Cast to nanoseconds
    const auto dur_nano = std::chrono::duration_cast<std::chrono::nanoseconds> (start_duration);

    return static_cast<uint64_t> (dur_nano.count());
}

uint64_t cycle_ns() {
    #if defined(__x86_64__)
    return __rdtsc();
    #elif defined(__aarch64__)
    uint64_t c;
    asm volatile("mrs %0, cntvct_el0" : "=r"(c));
    return c;
    #endif
}

Histogram::Histogram() {
    min = UINT64_MAX;
    max = 0;
    total = 0;
}

void Histogram::record(uint64_t ns) {
    // Calculate bucket index
    auto index = std::bit_width(ns) - 1;

    if (ns == 0) {
        index = 0;
    }

    counts[static_cast<size_t>(index)]++;

    // Update min, max, and count
    min = ns < min ? ns : min;
    max = ns > max ? ns : max;
    total++;
}

uint64_t Histogram::percentile(double p) {
    double cap = static_cast<double>(total) * p;

    double cur_total = 0.0;
    uint64_t index = 0;
    while (cur_total <= cap && index < NUM_BUCKET) {
        cur_total += static_cast<double>(counts.at(index));
        index++;
    }
    return (1UL << (index - 1));
}

namespace {

// 1234567 -> "1,234,567"
std::string with_commas(uint64_t n) {
    std::string digits = std::to_string(n);
    std::string out;
    out.reserve(digits.size() + digits.size() / 3);
    for (size_t i = 0; i < digits.size(); ++i) {
        const size_t remaining = digits.size() - i;
        if (i != 0 && remaining % 3 == 0) {
            out += ',';
        }
        out += digits[i];
    }
    return out;
}

}  // namespace

void Histogram::print(std::ostream& os) {
    constexpr int label_w = 10;
    constexpr int value_w = 16;

    const auto row = [&](const char* label, uint64_t value) {
        os << "  " << std::left << std::setw(label_w) << label
           << std::right << std::setw(value_w) << with_commas(value) << '\n';
    };
    const auto rule = [&] {
        os << "  " << std::string(label_w, '-') << std::string(value_w, '-') << '\n';
    };

    os << "Latency histogram  (" << with_commas(get_total()) << " samples)\n";
    rule();
    os << "  " << std::left << std::setw(label_w) << "quantile"
       << std::right << std::setw(value_w) << "ns (>=)" << '\n';
    rule();
    row("p50",   percentile(0.5));
    row("p90",   percentile(0.9));
    row("p99",   percentile(0.99));
    row("p99.9", percentile(0.999));
    row("max",   get_max());
    rule();
    os << "  (quantiles are lower bounds of power-of-two buckets)\n";
}

uint64_t Histogram::get_count(uint64_t index) {
    return counts.at(index);
}

uint64_t Histogram::get_total() {
    return total;
}

uint64_t Histogram::get_min() {
    return min;
}

uint64_t Histogram::get_max() {
    return max;
}
