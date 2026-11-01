#include <foundations/my_vector.hpp>
#include <foundations/my_string.hpp>

#include <benchmark/benchmark.h>

#include <vector>

static void BM_My_Vector_Small(benchmark::State& state) {
    for (auto _ : state) {
        MyVector<MyString> vec{8};
        for (int i = 0; i < 1000; i++) {
            vec.push_back(MyString("Hello! This is a test for long strings."));
        }
    }
}

BENCHMARK(BM_My_Vector_Small);

static void BM_Std_Vector_Small(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<MyString> vec;
        vec.reserve(8);
        for (int i = 0; i < 1000; i++) {
            vec.push_back(MyString("Hello! This is a test for long strings."));
        }
    }
}

BENCHMARK(BM_Std_Vector_Small);

static void BM_My_Vector_Large(benchmark::State& state) {
    for (auto _ : state) {
        MyVector<MyString> vec{8};
        for (int i = 0; i < 1000000; i++) {
            vec.push_back(MyString("Hello! This is a test for long strings."));
        }
    }
}

BENCHMARK(BM_My_Vector_Large);

static void BM_Std_Vector_Large(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<MyString> vec;
        vec.reserve(8);
        for (int i = 0; i < 1000000; i++) {
            vec.push_back(MyString("Hello! This is a test for long strings."));
        }
    }
}

BENCHMARK(BM_Std_Vector_Large);

BENCHMARK_MAIN();