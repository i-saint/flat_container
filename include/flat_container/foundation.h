#pragma once
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <iterator>
#include <type_traits>

#ifdef _DEBUG
#   if !defined(FC_ENABLE_CAPACITY_CHECK)
#       define FC_ENABLE_CAPACITY_CHECK
#   endif
#endif

#define fc_require(...) std::enable_if_t<__VA_ARGS__, bool> = true

namespace ist {

template <typename T, typename = void>
constexpr bool is_dynamic_memory_v = false;
template <typename T>
constexpr bool is_dynamic_memory_v<T, std::enable_if_t<T::is_dynamic_memory>> = true;

template <typename T, typename = void>
constexpr bool is_memory_view_v = false;
template <typename T>
constexpr bool is_memory_view_v<T, std::enable_if_t<T::is_memory_view>> = true;

template <typename T, typename = void>
constexpr bool is_trivially_swappable_v = false;
template <typename T>
constexpr bool is_trivially_swappable_v<T, std::enable_if_t<T::is_trivially_swappable>> = true;

template<typename T, typename = void>
constexpr bool is_iterator_v = false;
template<typename T>
constexpr bool is_iterator_v<T, typename std::enable_if_t<!std::is_same_v<typename std::iterator_traits<T>::value_type, void>>> = true;


template<class T>
class dynamic_memory
{
public:
    static const bool is_dynamic_memory = true;
    static const bool is_trivially_swappable = true;

    dynamic_memory() {}
    dynamic_memory(const dynamic_memory&) = delete;
    dynamic_memory(dynamic_memory&&) = delete;
   ~dynamic_memory() { release(); }
    constexpr size_t capacity() noexcept { return capacity_; }
    constexpr size_t size() const noexcept { return size_; }
    constexpr T* data() noexcept { return data_; }
    constexpr const T* data() const noexcept { return data_; }

    void reserve(size_t n)
    {
        if (n <= capacity_) {
            return;
        }
        size_t new_capaity = std::max<size_t>(n, capacity_ * 2);
        reallocate(new_capaity);
    }

    void shrink_to_fit()
    {
        reallocate(size_);
    }

protected:
    T* allocate(size_t size)
    {
        return (T*)std::malloc(sizeof(T) * size);
    }
    void deallocate(void *addr)
    {
        std::free(addr);
    }

    void reallocate(size_t new_capaity)
    {
        T* new_data = allocate(new_capaity);
        if constexpr (std::is_trivially_constructible_v<T>) {
            std::memcpy(new_data, data_, sizeof(T) * size_);
        }
        else {
            for (size_t i = 0; i < size_; ++i) {
                new (new_data + i) T(std::move(data_[i]));
                data_[i].~T();
            }
        }

        deallocate(data_);
        capacity_ = new_capaity;
        data_ = new_data;
    }

    void release()
    {
        if constexpr (!std::is_trivially_constructible_v<T>) {
            for (size_t i = 0; i < size_; ++i) {
                data_[i].~T();
            }
        }
        deallocate(data_);

        capacity_ = size_ = 0;
        data_ = nullptr;
    }

    size_t capacity_ = 0;
    size_t size_ = 0;
    T* data_ = nullptr;
};

template<class T, size_t Capacity>
class fixed_memory
{
public:
    // suppress warning for uninitialized members
#pragma warning(disable:26495)
    fixed_memory() {}
#pragma warning(default:26495)
    fixed_memory(const fixed_memory&) = delete;
    fixed_memory(fixed_memory&&) = delete;
   ~fixed_memory() {}

    constexpr static size_t capacity() noexcept { return capacity_; }
    constexpr size_t size() const noexcept { return size_; }
    constexpr T* data() noexcept { return data_; }
    constexpr const T* data() const noexcept { return data_; }

    constexpr void reserve(size_t n) {}
    constexpr void shrink_to_fit() {}

protected:
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
    memory_view(const memory_view&) = delete;
    memory_view(memory_view&&) = delete;
    memory_view(void* data, size_t capacity, size_t size = 0) : capacity_(capacity), size_(size), data_((T*)data) {}
    ~memory_view() {}

    constexpr size_t capacity() noexcept { return capacity_; }
    constexpr size_t size() const noexcept { return size_; }
    constexpr T* data() noexcept { return data_; }
    constexpr const T* data() const noexcept { return data_; }

    constexpr void reserve(size_t n) {}
    constexpr void shrink_to_fit() {}

protected:
    size_t capacity_ = 0;
    size_t size_ = 0;
    T* data_ = nullptr;
};


template<class T>
class constant_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    using ptrdiff_t = std::ptrdiff_t;

    constant_iterator(T& v, ptrdiff_t count = 0) : ref_(v), count_(count) {}
    constant_iterator(const constant_iterator&) = default;
    constant_iterator(constant_iterator&&) = default;
    constant_iterator& operator=(const constant_iterator&) = default;
    constant_iterator& operator=(constant_iterator&&) = default;

    reference operator*() { return ref_; }
    pointer operator->() { return &ref_; }
    constant_iterator& operator++() { ++count_; return *this; }
    constant_iterator& operator--() { --count_; return *this; }
    constant_iterator operator++(int) { auto ret = *this; ++count_; return ret; }
    constant_iterator operator--(int) { auto ret = *this; --count_; return ret; }
    constant_iterator& operator+=(ptrdiff_t n) { count_ += n; return *this; }
    constant_iterator& operator-=(ptrdiff_t n) { count_ -= n; return *this; }
    constant_iterator operator+(ptrdiff_t n) const { return { ref_, count_ + n }; }
    constant_iterator operator-(ptrdiff_t n) const { return { ref_, count_ - n }; }
    difference_type operator-(const constant_iterator& r) const { return count_ - r.count_; }
    bool operator<(const constant_iterator& r) const { return count_ < r.count_; }
    bool operator>(const constant_iterator& r) const { return count_ > r.count_; }
    bool operator==(const constant_iterator& r) const { return count_ == r.count_; }
    bool operator!=(const constant_iterator& r) const { return count_ != r.count_; }

    T& ref_;
    ptrdiff_t count_;
};
template<class T>
inline auto make_constant_iterator(T& v) {
    return constant_iterator<T>{v};
}

} // namespace ist

