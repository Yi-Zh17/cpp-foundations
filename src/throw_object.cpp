#include "foundations/throw_object.hpp"

static int counter = 0;

ThrowObject::ThrowObject(int i) {
    data = new int(i);
}

ThrowObject::~ThrowObject() {
    if (data) {
        delete data;
    }
}

ThrowObject::ThrowObject(const ThrowObject& another) {
    if (counter >= 4) {
        throw "Copy constructed 5 objects.";
    }
    data = new int(*(another.data));
    counter++;
}

ThrowObject& ThrowObject::operator=(const ThrowObject& another) {
    if (this == &another) {
        return *this;
    }

    if (counter >= 4) {
        throw "Copy constructed 5 objects.";
    }

    if (data) {
        delete data;
    }
    
    data = new int(*(another.data));
    counter++;
    return *this;
}

int* ThrowObject::get() const { return data; }
