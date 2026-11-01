#include "foundations/my_vector.hpp"
#include "foundations/my_string.hpp"
#include "foundations/throw_object.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <iostream>

TEST(test_constructor, construct_vector) {
    ASSERT_NO_THROW(MyVector<int> int_vec{8};);
    ASSERT_NO_THROW(MyVector<MyString> str_vec{8};);
}

TEST(test_push_back, can_push_back_int) {
    MyVector<int> vec{8};

    ASSERT_NO_THROW(vec.push_back(1););
    ASSERT_NO_THROW(vec.push_back(10000););
}

TEST(test_push_back, can_push_back_custom) {
    MyVector<MyString> vec{8};
    MyString string_1{"Hello!"};
    ASSERT_NO_THROW(vec.push_back(string_1););

    ASSERT_STREQ(vec[0].c_str(), "Hello!");
}

TEST(test_size, size_returns_correct_number) {
    MyVector<int> vec{8};
    ASSERT_EQ(vec.size(), 0);

    vec.push_back(1);
    ASSERT_EQ(vec.size(), 1);

    for (int i = 0; i < 7; i++) {
        vec.push_back(i);
    }

    ASSERT_EQ(vec.size(), 8);
}

TEST(test_access_operator, returns_correct_int) {
    MyVector<int> vec{8};
    vec.push_back(1);

    ASSERT_EQ(vec[0], 1);

    vec.push_back(10000);
    ASSERT_EQ(vec[1], 10000);
}

TEST(test_access_operator, returns_correct_custom) {
    MyVector<MyString> vec{8};
    MyString string_1{"Hello!"};
    vec.push_back(string_1);

    ASSERT_STREQ(vec[0].c_str(), "Hello!");
}

TEST(test_resize, resize_succeed_int) {
    MyVector<int> vec{8};
    for (int i = 0; i < 1000; i++) {
        ASSERT_NO_THROW(vec.push_back(i););
    }
}

TEST(test_resize, resize_succeed_custom) {
    MyVector<MyString> vec{8};
    MyString str_1 {"Hello!"};

    for (int i = 0; i < 1000; i++) {
        ASSERT_NO_THROW(vec.push_back(str_1););
    }
}

TEST(test_emplace_back, emplace_back_succeed) {
    MyVector<MyString> vec{8};

    for (int i = 0; i < 1000; i++) {
        ASSERT_NO_THROW(vec.emplace_back("Hello!"););
    }

    ASSERT_EQ(MyString::normal(), 1000);
    ASSERT_EQ(MyString::copy(), 0);
    ASSERT_EQ(MyString::move(), 1016);
}

TEST(test_iterator, sort_by_iterator) {
    MyVector<int> vec{8};

    vec.push_back(4);
    vec.push_back(5);
    vec.push_back(1);
    vec.push_back(9);
    vec.push_back(0);
    vec.push_back(3);
    vec.push_back(7);
    vec.push_back(5);

    std::sort(vec.begin(), vec.end());

    ASSERT_EQ(vec[0], 0);
    ASSERT_EQ(vec[1], 1);
    ASSERT_EQ(vec[2], 3);
    ASSERT_EQ(vec[3], 4);
    ASSERT_EQ(vec[4], 5);
    ASSERT_EQ(vec[5], 5);
    ASSERT_EQ(vec[6], 7);
    ASSERT_EQ(vec[7], 9);
}

TEST(test_iterator, sort_by_iterator_const) {
    MyVector<int> vec{8};

    vec.push_back(4);
    vec.push_back(5);
    vec.push_back(1);
    vec.push_back(9);
    vec.push_back(0);
    vec.push_back(3);
    vec.push_back(7);
    vec.push_back(5);

    const MyVector<int>& cref = vec;

    int sum = 0;
    for (const int i : cref) {
        sum += i;
    }

    ASSERT_EQ(sum, 34);
}

TEST(exception_safety, old_intact_when_throw_during_growth) {
    MyVector<ThrowObject> vec{1};

    ASSERT_NO_THROW(vec.push_back(ThrowObject(0)););
    ASSERT_NO_THROW(vec.push_back(ThrowObject(1)););

    ASSERT_THROW(vec.push_back(ThrowObject(2)), const char*);

    ASSERT_EQ(*(vec[0].get()), 0);
    ASSERT_EQ(*(vec[1].get()), 1);
}