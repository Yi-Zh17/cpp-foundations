#include "foundations/my_string.hpp"

#include <cstring>

MyString::MyString() {
    // Empty string
    this->length = 0;
    inline_buf[0] = '\0';
    heap.capacity = 0;
    heap.string = nullptr;
}

MyString::MyString(const char* str) {
    // Get length
    length = strlen(str);

    if (length > SSO_SIZE) {
        small = false;
        heap.capacity = length;
        heap.string = new char[length + 1];
        strcpy(heap.string, str);
    } else {
        strcpy(inline_buf, str);
    }
}

MyString::MyString(const MyString& another) {
    // Copy length
    length = another.size();

    if (length > SSO_SIZE) {
        small = false;
        heap.string = new char[length + 1];
        heap.capacity = length;
        strcpy(heap.string, another.heap.string);
    } else {
        strcpy(inline_buf, another.inline_buf);
    }
}

MyString::MyString(MyString&& another) noexcept {
    small = another.small;
    length = another.length;

    if (another.small) {
        strcpy(inline_buf, another.inline_buf);
        another.inline_buf[0] = '\0';
    } else {
        heap.string = another.heap.string;
        heap.capacity = another.heap.capacity;
        another.heap.string = nullptr;
        another.heap.capacity = 0;
    }
    another.length = 0;
}

MyString::MyString(std::string_view str_view) {
    length = str_view.size();

    if (length > SSO_SIZE) {
        small = false;
        heap.string = new char[length + 1];
        heap.capacity = length;
        strcpy(heap.string, str_view.data());
    } else {
        strcpy(inline_buf, str_view.data());
    }
}

MyString::~MyString() {
    if (!small && heap.string) {
        small = true;
        heap.capacity = 0;
        delete[] heap.string;
    }
    length = 0;
}

MyString& MyString::operator=(const MyString& another) {
    // Guard against self-assignment
    if (this == &another) {
        return *this;
    }

    // Delete old data
    if (length != 0) {
        length = another.size();
    }

    if (length > SSO_SIZE) {
        small = false;
        if (heap.string != nullptr) {
            delete[] heap.string;
        }
        // Assign new string
        heap.string = new char[length + 1];
        strcpy(heap.string, another.c_str());
        heap.capacity = length;
    } else {
        small = true;
        if (inline_buf[0] != '\0') {
            inline_buf[0] = '\0';
        }
        strcpy(inline_buf, another.inline_buf);
    }
    
    return *this;
}

MyString& MyString::operator=(MyString&& another) noexcept {
    if (inline_buf[0] != '\0' && small) {
        inline_buf[0] = '\0';
    }
    if (heap.string && (!small)) {
        delete[] heap.string;
    }
    length = another.length;

    if (another.small) {
        small = true;
        strcpy(inline_buf, another.inline_buf);
        another.inline_buf[0] = '\0';
    } else {
        small = false;
        heap.string = another.heap.string;
        heap.capacity = another.heap.capacity;
        another.heap.string = nullptr;
        another.heap.capacity = 0;
    }

    another.length = 0;

    return *this;
}

bool MyString::operator==(const MyString& another) const {
    if (this->size() != another.size()) {
        return false;
    }
    
    if (this->size() == 0) {
        return true;
    }

    return memcmp(this->c_str(), another.c_str(), this->size()) == 0;
}

MyString MyString::operator+(const MyString& another) {
    size_t new_len = length + another.length;
    char* new_str = new char[new_len + 1];
    strcpy(new_str, c_str());
    strcpy(new_str + length, another.c_str());

    MyString temp{new_str};
    delete[] new_str;
    return temp;
}

size_t MyString::size() const {
    return length;
}

const char* MyString::c_str() const {
    if (small) {
        return inline_buf;
    }
    return heap.string;
}

