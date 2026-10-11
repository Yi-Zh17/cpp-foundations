# One executable per benchmark source, so a single file can be built and run
# on its own (./benchmark.sh release -f bench_histogram). New bench_*.cpp files
# are picked up at the next configure.
file(GLOB bench_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/bench_*.cpp")

set(bench_targets)
foreach(src IN LISTS bench_sources)
    get_filename_component(name "${src}" NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name}
        PRIVATE foundations
                foundations_warnings
                benchmark::benchmark
    )
    list(APPEND bench_targets ${name})
endforeach()

# Build every benchmark executable without touching tests or exercises.
add_custom_target(all_benchmarks DEPENDS ${bench_targets})
