#include "types.hpp"

namespace core {

template <FloatingPoint T>
class avg_counter {
public:
    avg_counter(size_t max_count): _max_count(static_cast<T>(max_count)) {}

    void update(T value) {
        _value = (_value * _count + value) / (_count + 1);
        if (_count + 1 < _max_count)
            _count += 1;
    }

    T value() const {
        return _value;
    }

private:
    T _value = 0;
    T _count = 0.f;
    T _max_count = 1.f;
};

}
