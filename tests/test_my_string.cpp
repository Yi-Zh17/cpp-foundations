#include "foundations/my_string.hpp"

#include <gtest/gtest.h>

TEST(MyString, DefaultConstructedIsEmpty) {
    const foundations::MyString s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0U);
}

TEST(MyString, KnowsItsLength) {
    const foundations::MyString s("hello");
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(s.size(), 5U);
}
