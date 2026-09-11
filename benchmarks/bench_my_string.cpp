#include "foundations/my_string.hpp"

#include <benchmark/benchmark.h>

#include <string>

static void BM_MyStringConstruct(benchmark::State& state) {
    for (auto _ : state) {
        foundations::MyString s("the quick brown fox");
        benchmark::DoNotOptimize(s);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_MyStringConstruct);

static void BM_StdStringConstruct(benchmark::State& state) {
    for (auto _ : state) {
        std::string s("the quick brown fox");
        benchmark::DoNotOptimize(s);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_StdStringConstruct);

BENCHMARK_MAIN();
