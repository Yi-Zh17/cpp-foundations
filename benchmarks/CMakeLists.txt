add_executable(foundations_bench
    bench_custom_clock.cpp
)
target_link_libraries(foundations_bench
    PRIVATE foundations
            foundations_warnings
            benchmark::benchmark
)
