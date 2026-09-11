#include "foundations/my_string.hpp"

#include <cstring>

namespace foundations {

MyString::MyString(const char* s) : size_(s != nullptr ? std::strlen(s) : 0) {}

}  // namespace foundations
