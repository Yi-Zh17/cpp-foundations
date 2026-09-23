add_executable(foundations_tests
    test_custom_clock.cpp
)
target_link_libraries(foundations_tests
    PRIVATE foundations
            foundations_warnings
            GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(foundations_tests)
