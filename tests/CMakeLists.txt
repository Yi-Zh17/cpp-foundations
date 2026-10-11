# One executable per test source, so a single file can be built and run on
# its own (./test.sh debug -f test_my_string). New test_*.cpp files are picked
# up at the next configure; the scripts always reconfigure, so nothing here
# needs editing when a file is added.
file(GLOB test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")

include(GoogleTest)

set(test_targets)
foreach(src IN LISTS test_sources)
    get_filename_component(name "${src}" NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name}
        PRIVATE foundations
                foundations_warnings
                GTest::gtest_main
    )
    # Label every discovered test with its file name so ctest can select a file.
    gtest_discover_tests(${name} PROPERTIES LABELS ${name})
    list(APPEND test_targets ${name})
endforeach()

# Build every test executable without touching benchmarks or exercises.
add_custom_target(all_tests DEPENDS ${test_targets})
