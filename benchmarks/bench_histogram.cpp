#include <foundations/histogram.hpp>

#include <benchmark/benchmark.h>
#include <string>

static void BM_CustomClock(benchmark::State& state) {
    for (auto _ : state) {
        now_ns();
    }
}
BENCHMARK(BM_CustomClock);

static void BM_Record(benchmark::State& state) {
    Histogram hist{};
    for (auto _ : state) {
        hist.record(1);
    }
}
BENCHMARK(BM_Record);

BENCHMARK_MAIN();