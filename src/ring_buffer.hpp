#pragma once

#include <stddef.h>
#include <cstddef>
#include <iterator>

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

    class ConstIterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using pointer           = const T*;
            using reference         = const T&;
        private:
            const RingBuffer* _buf;
            size_t _idx;
        public:
            ConstIterator() : _buf(nullptr), _idx(0) {}
            ConstIterator(const RingBuffer* buf, size_t idx) : _buf(buf), _idx(idx) {}
            reference operator*() const { return (*_buf)[_idx]; }
            pointer operator->() const { return &(operator*()); }
            ConstIterator& operator++() {
                _idx++;
                return *this;
            }
            ConstIterator operator++(int) {
                ConstIterator tmp = *this;
                ++(*this);
                return tmp;
            }
            bool operator==(const ConstIterator& other) const { return _buf == other._buf && _idx == other._idx; }
            bool operator!=(const ConstIterator& other) const { return !(*this == other); }
        };
        ConstIterator begin() const { return ConstIterator(this, 0); }
        ConstIterator end() const   { return ConstIterator(this, _size); }
        ConstIterator cbegin() const { return ConstIterator(this, 0); }
        ConstIterator cend() const   { return ConstIterator(this, _size); }
};
