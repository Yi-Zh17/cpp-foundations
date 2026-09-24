#include <foundations/histogram.hpp>

int main () {
    Histogram hist_now{};

    uint64_t before = now_ns();
    uint64_t after = now_ns();
    
    for (uint64_t i = 0; i < 1000000; i++) {
        hist_now.record(after - before);
        before = now_ns();
        after = now_ns();
    }

    hist_now.print(std::cout);

    Histogram hist_cycle{};

    before = cycle_ns();
    after = cycle_ns();

    for (uint64_t i = 0; i < 1000000; i++) {
        hist_cycle.record(after - before);
        before = cycle_ns();
        after = cycle_ns();
    }

    hist_cycle.print(std::cout);
}