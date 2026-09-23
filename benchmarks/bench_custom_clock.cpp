#include <foundations/custom_clock.hpp>

#include <benchmark/benchmark.h>
#include <string>

static void BM_CustomClock(benchmark::State& state) {
    for (auto _ : state) {
        auto cur_time = now_ns();
        benchmark::DoNotOptimize(cur_time);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_CustomClock);

BENCHMARK_MAIN();