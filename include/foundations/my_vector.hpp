#ifndef MY_VECTOR_H_
#define MY_VECTOR_H_

#include <stddef.h>
#include <utility>
#include <new>

template <typename T>
class MyVector {
public:
    MyVector<T>(size_t cap);
    ~MyVector();

    void push_back(const T& obj);

    template <typename... Args>
    void emplace_back(Args&&... args);

    size_t size();
    T& operator[] (size_t index);

    T* begin();
    const T* begin() const;
    T* end();
    const T* end() const;
private:
    size_t num;
    size_t cap;
    T* data;

    void grow();
};

template <typename T>
MyVector<T>::MyVector(size_t cap) {
    data = static_cast<T*>(::operator new(cap * sizeof(T)));
    this->cap = cap;
    num = 0;
}

template <typename T>
MyVector<T>::~MyVector() {
    if (data) {
        for (size_t i = 0; i < num; i++) {
            data[i].~T();
        }
        ::operator delete(data);
    }
    cap = 0;
    num = 0;
}

template <typename T>
void MyVector<T>::push_back(const T& obj) {
    if (num == cap) {
        grow();
    }
    new (data + num) T(obj);
    num++;
}

template <typename T>
template <typename... Args>
void MyVector<T>::emplace_back(Args&&... args) {
    if (num == cap) {
        grow();
    }
    new (data + num) T(std::forward<Args>(args)...);
    num++;
}

template <typename T>
size_t MyVector<T>::size() {
    return num;
}

template <typename T>
T& MyVector<T>::operator[](size_t index) {
    T* ptr = data + index;
    return *ptr;
}

template <typename T>
T* MyVector<T>::begin() {
    return data;
}

template <typename T>
T* MyVector<T>::end() {
    return data + num;
}

template <typename T>
const T* MyVector<T>::begin() const {
    return data;
}

template <typename T>
const T* MyVector<T>::end() const {
    return data + num;
}

template <typename T>
void MyVector<T>::grow() {
    size_t new_cap = cap * 2;
    if (cap == 0) {
        new_cap = 1;
    }
    T* new_data = static_cast<T*>(::operator new(new_cap * sizeof(T)));

    size_t i = 0;
    try {
        for (; i < cap; i++) {
            new (new_data + i) T(std::move_if_noexcept(data[i]));
        }
    } catch (...) {
        for (; i > 0; i--) {
            new_data[i - 1].~T();
        }
        ::operator delete(new_data);
        throw;
    }

    for (size_t j = 0; j < cap; j++) {
        data[j].~T();
    }

    ::operator delete(data);
    cap  = new_cap;
    data = new_data;
}

#endif
