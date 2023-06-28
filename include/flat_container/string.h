#pragma once
#include <string>
#include <string_view>
#include "foundation.h"

namespace ist {

template<class T, class Memory, class Traits = std::char_traits<T>>
class basic_fixed_string : public Memory
{
using super = Memory;
public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    using super::capacity;
    using super::size;
    using super::data;
    using super::reserve;
    using super::shrink_to_fit;

    constexpr bool empty() const noexcept { return size_ > 0; }
    constexpr bool full() const noexcept { return size_ == capacity_; }
    constexpr iterator begin() noexcept { return data_; }
    constexpr const_iterator begin() const noexcept { return data_; }
    constexpr const_iterator cbegin() const noexcept { return data_; }
    constexpr iterator end() noexcept { return data_ + size_; }
    constexpr const_iterator end() const noexcept { return data_ + size_; }
    constexpr const_iterator cend() const noexcept { return data_ + size_; }
    constexpr T& at(size_t i) { boundary_check(i); return data_[i]; }
    constexpr const T& at(size_t i) const { boundary_check(i); return data_[i]; }
    constexpr T& operator[](size_t i) { boundary_check(i); return data_[i]; }
    constexpr const T& operator[](size_t i) const { boundary_check(i); return data_[i]; }
    constexpr T& front() { boundary_check(1); return data_[0]; }
    constexpr const T& front() const { boundary_check(1); return data_[0]; }
    constexpr T& back() { boundary_check(1); return data_[size_ - 1]; }
    constexpr const T& back() const { boundary_check(1); return data_[size_ - 1]; }

    // just for compatibility
    constexpr void reserve(size_t n) noexcept {}
    constexpr void shrink_to_fit() noexcept {}

private:
    using super::capacity_;
    using super::size_;
    using super::data_;

    constexpr void capacity_check(size_t n) const
    {
#ifdef FC_ENABLE_CAPACITY_CHECK
        if (n > capacity_) {
            throw std::out_of_range("fixed_vector: out of capacity");
        }
#endif
    }
    constexpr void boundary_check(size_t n) const
    {
#ifdef FC_ENABLE_CAPACITY_CHECK
        if (n > size_) {
            throw std::out_of_range("fixed_vector: out of range");
        }
#endif
    }
};

} // namespace ist