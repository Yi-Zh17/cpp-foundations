#ifndef MY_STRING_H_
#define MY_STRING_H_

#include <cstddef>
#include <string_view>

constexpr size_t SSO_SIZE = 16;

class MyString {
public:
    MyString(); // Default constructor
    MyString(const char* str);                 
    MyString(const MyString& another);                     // Copy constructor
    MyString(MyString&& another) noexcept;           // Move constructor
    explicit MyString(std::string_view str_view);

    ~MyString();

    MyString& operator=(const MyString& another);
    MyString& operator=(MyString&& another) noexcept;

    bool operator==(const MyString& another) const;
    MyString operator+(const MyString& another);

    /* Getters */
    size_t size() const;
    const char* c_str() const;

private:
    bool small = true;
    size_t length;

    struct large {
        size_t capacity;
        char* string;
    };

    union {
        large heap;
        char inline_buf[SSO_SIZE + 1]; // Include null terminator
    };
};

#endif