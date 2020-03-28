#pragma once

#include "../types.hpp"

namespace core
{
    template <typename T>
    class MemStack {
    public:
        MemStack(typename deque<T>::size_type max_size = 100): _max_size(max_size) {}

        void push(const T& val) {
            _data.push_front(val);

            while (_data.size() > _max_size)
                _data.pop_back();
        }

        T& front() { return _data.front(); }
        T& back()  { return _data.back(); }
        
        const T& front() const { return _data.front(); }
        const T& back()  const { return _data.back(); }

        auto begin()       { return _data.begin(); }
        auto begin() const { return _data.begin(); }

        auto end()       { return _data.end(); }
        auto end() const { return _data.end(); }

        auto size() const {
            return _data.size();
        }

        auto max_size() const {
            return _max_size;
        }

        void max_size(typename deque<T>::size_type max_size) {
            _max_size = max_size;

            while (_data.size() > _max_size)
                _data.pop_back();
        }

    private:
        deque<T>                     _data;
        typename deque<T>::size_type _max_size;
    };
} // namespace core

