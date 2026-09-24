#include <foundations/histogram.hpp>

#include <gtest/gtest.h>

TEST(custom_clock, later_larger_than_earlier) {
    uint64_t before = now_ns();
    uint64_t after = now_ns();
    EXPECT_GE(after, before);
}

TEST(record, nanos_land_in_the_right_bucket) {
    Histogram hist{};

    hist.record(1);
    hist.record(2);
    hist.record(3);
    hist.record(700);

    EXPECT_EQ(hist.get_count(0), 1);
    EXPECT_EQ(hist.get_count(1), 2);
    EXPECT_EQ(hist.get_count(2), 0);
    EXPECT_EQ(hist.get_count(3), 0);
    EXPECT_EQ(hist.get_count(9), 1);
    EXPECT_EQ(hist.get_count(63), 0);
}

TEST(percentile, percentiles_report_correct_lower_bound) {
    Histogram hist{};

    for (uint64_t i = 0; i < 1000; i++) {
        hist.record(i);
    }

    EXPECT_EQ(hist.percentile(0.01), 8);
    EXPECT_EQ(hist.percentile(0.1), 64);
    EXPECT_EQ(hist.percentile(0.2), 128);   
    EXPECT_EQ(hist.percentile(0.3), 256);
    EXPECT_EQ(hist.percentile(0.5), 256);
    EXPECT_EQ(hist.percentile(0.99), 512);
}
