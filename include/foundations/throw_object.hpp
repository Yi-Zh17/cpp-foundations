class ThrowObject {
public:
    ThrowObject (int i);
    ~ThrowObject();
    ThrowObject (const ThrowObject& another);
    ThrowObject& operator= (const ThrowObject& another);

    int* get() const;

private:
    int* data;
};