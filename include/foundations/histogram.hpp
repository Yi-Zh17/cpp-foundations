#include <chrono>
#include <array>
#include <iostream>

// Macros
constexpr uint64_t NUM_BUCKET = 64;

class Histogram {
public:
    Histogram();

    /**
     * Record a measured time into buckets.
     */
    void record(uint64_t ns);

    /**
     * Return the time bound of a percentile.
     */
    uint64_t percentile(double p);

    /**
     * Prints the custom percentiles and stats.
     */
    void print(std::ostream&);

    /**
     * Count-in-a-bucket getter function.
     */
    uint64_t get_count(uint64_t index);

    /**
     * Get total number of records.
     */
    uint64_t get_total();

    /**
     * Get min.
     */
    uint64_t get_min();

    /**
     * Get max.
     */
    uint64_t get_max();


private:
    std::array<uint64_t, NUM_BUCKET> counts{};
    uint64_t min;
    uint64_t max;
    uint64_t total;
};

uint64_t now_ns();

uint64_t cycle_ns();