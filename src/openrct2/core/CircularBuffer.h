/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

template<typename TType, size_t TMax>
class CircularBuffer
{
public:
    using value_type = TType;
    using pointer = TType*;
    using const_pointer = const TType*;
    using reference = TType&;
    using const_reference = const TType&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    reference front()
    {
        return _elements[_head];
    }

    const_reference front() const
    {
        return _elements[_head];
    }

    reference back()
    {
        return _elements[_tail];
    }

    const_reference back() const
    {
        return _elements[_tail];
    }

    reference operator[](size_type idx)
    {
        idx = (_head + idx) % capacity();
        return _elements[idx];
    }

    const_reference operator[](size_type idx) const
    {
        idx = (_head + idx) % capacity();
        return _elements[idx];
    }

    void clear()
    {
        _head = 0;
        _tail = 0;
        _size = 0;
    }

    size_type size() const
    {
        return _size;
    }

    bool empty() const
    {
        return _size == 0;
    }

    constexpr size_type capacity() const
    {
        return _elements.size();
    }

    template<typename U>
        requires std::convertible_to<U, value_type>
    void push_back(U&& val)
    {
        if (_size == 0)
        {
            _tail = _head;
        }
        else
        {
            _tail = (_tail + 1) % capacity();
            if (_size == capacity())
                _head = (_head + 1) % capacity();
        }
        _elements[_tail] = std::forward<U>(val);
        if (_size < capacity())
            _size++;
    }

private:
    size_t _head = 0;
    size_t _tail = 0;
    size_t _size = 0;
    std::array<TType, TMax> _elements;
};
