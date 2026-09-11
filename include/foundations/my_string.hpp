#pragma once

#include <cstddef>

namespace foundations {

// Placeholder for Phase 1, exercise 1. Replace the body with your own
// heap-owning string: rule of five, then small-string optimisation.
class MyString {
public:
    MyString() = default;
    explicit MyString(const char* s);

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

private:
    std::size_t size_ = 0;
};

}  // namespace foundations
