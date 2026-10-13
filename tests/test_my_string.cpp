#include "foundations/my_string.hpp"
#include "gtest/gtest.h"

#include <utility>
#include <string_view>


TEST(test_constructor, constructor_creates_a_string) {
    ASSERT_NO_THROW(
        MyString string_1 {"Hello world!"};
    );
};

TEST(test_size, returns_correct_length) {
    MyString string_1 {"Hello world!"};

    ASSERT_EQ(string_1.size(), size_t{12});
}

TEST(test_c_str, returns_correct_c_style_string) {
    MyString string_1 {"Hello world!"};
    const char* c_str_1 = string_1.c_str();
    size_t length = strlen(c_str_1);

    ASSERT_EQ(*(c_str_1 + length), '\0');
    
    ASSERT_STREQ(c_str_1, "Hello world!");
}

TEST(test_copy_constructor, string_copy_double_free) {
    MyString string_1 {"Hello world!"};
    ASSERT_NO_THROW(MyString string_2  = string_1;);
}

TEST(test_copy_constructor, can_copy_const_or_temp) {
    const MyString string_1 {"Hello world!"};
    ASSERT_NO_THROW(MyString string_2 {string_1};);

    ASSERT_TRUE(string_1 == "Hello world!");
}

TEST(test_copy_assignment, copy_assignment_replace_old_data) {
    MyString string_1 {"Hello world!"};
    MyString string_2 = string_1;

    ASSERT_EQ(string_2.size(), size_t{12});
    ASSERT_STREQ(string_2.c_str(), "Hello world!");
}

TEST(test_copy_assignment, self_assignment_should_do_nothing) {
    MyString string_1 {"Hello world!"};

    ASSERT_NO_THROW(string_1 = string_1);
}

TEST(test_move_constructor, undefined_after_move_small) {
    MyString string_1 {"Hello world!"};
    MyString string_2 = std::move(string_1);

    ASSERT_EQ(string_1.c_str()[0], '\0');
    ASSERT_EQ(string_1.size(), 0);

    ASSERT_EQ(string_2.size(), size_t{12});
    ASSERT_STREQ(string_2.c_str(), "Hello world!");
}

TEST(test_move_constructor, undefined_after_move_large) {
    MyString string_1 {"This is a test string for a large case."};
    const char* original = string_1.c_str();

    MyString string_2 = std::move(string_1);
    const char* after = string_2.c_str();

    ASSERT_EQ(string_1.c_str(), nullptr);
    ASSERT_EQ(string_1.size(), 0);

    ASSERT_EQ(string_2.size(), size_t{39});
    ASSERT_STREQ(string_2.c_str(), "This is a test string for a large case.");

    ASSERT_EQ(original, after);
}

TEST(test_string_view_constructor, string_view_constructor_initialises_MyString) {
    std::string_view str_view = "Hello world!";
    ASSERT_NO_THROW(MyString string_1 {str_view};);
}

TEST(test_SSO, small_and_large_can_be_initialised) {
    ASSERT_NO_THROW(MyString string_1 {"Hello"};);
    ASSERT_NO_THROW(MyString string_2 {"world! This is a test string for large strings"};);
}

TEST(test_SSO, concatenation) {
    MyString string_1{"Hello "};
    MyString string_2 {"world! This is a test string for large strings"};

    ASSERT_NO_THROW(string_1 = string_1 + string_2;);
    ASSERT_STREQ(string_1.c_str(), "Hello world! This is a test string for large strings");
    ASSERT_EQ(string_1.size(), 52);
}