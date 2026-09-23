#include <foundations/custom_clock.hpp>

#include <gtest/gtest.h>

TEST(custom_clock, later_larger_than_earlier) {
    uint64_t before = now_ns();
    uint64_t after = now_ns();
    EXPECT_GT(after, before);
}