#pragma once

#include <stddef.h>

template <typename T, size_t N>
class RingBuffer {
    static_assert(N > 0, "RingBuffer needs a size of at least 1");

    T _data[N] = {};
    size_t _next = 0; 
    size_t _size = 0;
public:
    inline bool push(const T &value) {
        _data[_next] = value;
        _next = (_next + 1) % N;
        if( _size < N ) {
            _size++;
        }
        return _next == 0;
    }
    inline size_t size() const { return _size; }
    inline bool empty() const { return _size == 0; }
    inline bool full() const { return _size == N; }
    inline const T& operator[](size_t i) const { return _data[(_next + N - _size + i % N) % N]; }
    inline const T& latest() const { return _data[(_next + N - 1) % N]; }
};
