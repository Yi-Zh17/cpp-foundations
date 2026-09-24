#include <chrono>
#include <array>

// Macros
constexpr unsigned long NUM_BUCKET = 64;

class Histogram {
public:
    Histogram();
    void record(uint64_t ns);
    uint64_t get_count(unsigned long index);
private:
    std::array<uint64_t, NUM_BUCKET> counts{};
};

uint64_t now_ns();