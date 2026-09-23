cmake_minimum_required(VERSION 3.24)

project(cpp_foundations
        VERSION 0.1.0
        DESCRIPTION "C++ foundations: exercises, tests, benchmarks"
        LANGUAGES CXX)

# ---- Language settings -------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json so clangd / clang-tidy work in your editor.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Options -----------------------------------------------------------------

option(FOUNDATIONS_BUILD_TESTS "Build unit tests" ON)
option(FOUNDATIONS_BUILD_BENCHMARKS "Build benchmarks" ON)
option(FOUNDATIONS_WERROR "Treat warnings as errors" ON)

# ---- Warning flags, applied to our code only ---------------------------------
# An INTERFACE library carries flags without producing a binary. Link it into
# your targets; do NOT set these globally, or third-party code fails to build.

add_library(foundations_warnings INTERFACE)
target_compile_options(foundations_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow                # a local name hiding an outer one
    -Wconversion            # implicit narrowing
    -Wsign-conversion
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align            # casts that increase required alignment
    -Wunused
    -Woverloaded-virtual
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
)
if(FOUNDATIONS_WERROR)
    target_compile_options(foundations_warnings INTERFACE -Werror)
endif()

# ---- The library under study -------------------------------------------------

add_library(foundations
    src/custom_clock.cpp
)
target_include_directories(foundations PUBLIC include)
target_link_libraries(foundations PRIVATE foundations_warnings)

# ---- Dependencies ------------------------------------------------------------

include(FetchContent)

if(FOUNDATIONS_BUILD_TESTS)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.18.0
        GIT_SHALLOW    TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()

if(FOUNDATIONS_BUILD_BENCHMARKS)
    FetchContent_Declare(benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.5
        GIT_SHALLOW    TRUE
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    # Benchmark builds itself with -Werror; GCC 13 emits false-positive
    # maybe-uninitialized warnings from libstdc++ headers under sanitizers.
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)

    add_subdirectory(benchmarks)
endif()
