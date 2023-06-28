#pragma once
#include <initializer_list>
#include <type_traits>
#include <cstddef>
#ifdef _DEBUG
#   if !defined(FC_ENABLE_CAPACITY_CHECK)
#       define FC_ENABLE_CAPACITY_CHECK
#   endif
#endif


namespace ist {

template<class T, size_t Capacity>
class fixed_memory
{
public:
    // suppress warning for uninitialized members
#pragma warning(disable:26495)
    fixed_memory() {}
#pragma warning(default:26495)
    ~fixed_memory() {}
    constexpr static size_t capacity() noexcept { return capacity_; }
    constexpr size_t size() const noexcept { return size_; }
    constexpr T* data() noexcept { return (T*)buffer_; }
    constexpr const T* data() const noexcept { return (T*)buffer_; }

public:
    static const size_t capacity_ = Capacity;
    size_t size_ = 0;
    union {
        T data_[0]; // for debug
        std::byte buffer_[sizeof(T) * Capacity]; // uninitialized in intention
    };
};

template<class T>
class memory_view
{
public:
    static const bool is_memory_view = true;
    static const bool is_trivially_swappable = true;

    memory_view() {}
    memory_view(void* data, size_t capacity, size_t size = 0) : capacity_(capacity), size_(size), data_((T*)data) {}
    constexpr size_t capacity() noexcept { return capacity_; }
    constexpr size_t size() const noexcept { return size_; }
    constexpr T* data() noexcept { return data_; }
    constexpr const T* data() const noexcept { return data_; }

public:
    size_t capacity_ = 0;
    size_t size_ = 0;
    T* data_ = nullptr;
};

template <typename T, typename = void>
constexpr bool is_memory_view_v = false;
template <typename T>
constexpr bool is_memory_view_v<T, std::enable_if_t<T::is_memory_view>> = true;

template <typename T, typename = void>
constexpr bool is_trivially_swappable_v = false;
template <typename T>
constexpr bool is_trivially_swappable_v<T, std::enable_if_t<T::is_trivially_swappable>> = true;



template<class T, class Memory>
class basic_fixed_raw_vector : public Memory
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