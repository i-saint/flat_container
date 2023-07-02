#pragma once
#include <string>
#include <string_view>
#include <charconv>
#include <iterator>
#include "foundation.h"

namespace ist {

template<typename T, typename = void>
constexpr bool is_from_chars_available_v = false;
template<typename T>
constexpr bool is_from_chars_available_v<T, std::void_t<decltype(std::from_chars((const char*)nullptr, (const char*)nullptr, std::declval<T&>()))>> = true;


template<class T, class Memory, class Traits = std::char_traits<T>>
class basic_string : public Memory
{
using super = Memory;
public:
    using traits_type = Traits;

    constexpr basic_string() = default;
    constexpr basic_string(const basic_string& r) = default;
    constexpr basic_string(basic_string&& r) noexcept = default;
    constexpr basic_string& operator=(const basic_string& r) = default;
    constexpr basic_string& operator=(basic_string&& r) noexcept = default;

    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr basic_string(const T* v) { assign(v, v + traits_type::length(v)); }
    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr basic_string(const T* v, size_t n) { assign(v, v + n); }

    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr basic_string(std::initializer_list<T> r) { assign(r); }

    template<class Iter, bool view = is_memory_view_v<super>, fc_require(!view), fc_require(is_iterator_v<Iter>)>
    constexpr basic_string(Iter first, Iter last) { assign(first, last); }

    template<bool view = is_memory_view_v<super>, fc_require(view)>
    constexpr basic_string(void* data, size_t capacity, size_t size = 0)
        : super(data, capacity, size)
    {
    }

    using super::capacity;
    using super::size;
    using super::size_bytes;
    using super::data;
    using super::reserve;
    using super::shrink_to_fit;
    using super::clear;

    using super::empty;
    using super::full;
    using super::begin;
    using super::cbegin;
    using super::end;
    using super::cend;
    using super::at;
    using super::operator[];
    using super::front;
    using super::back;

    constexpr size_t length() const { return size(); }

    constexpr void resize(size_t n)
    {
        _resize(n, [&](T*) {}); // new elements are uninitialized
    }
    constexpr void resize(size_t n, T v)
    {
        _resize(n, [&](T* addr) { *addr = v; });
    }

    constexpr void push_back(const T& v)
    {
        _expand(1, [&](T* addr) { *addr = v; });
    }

    constexpr void pop_back()
    {
        _shrink(1);
    }

    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr void assign(Iter first, Iter last)
    {
        size_t n = std::distance(first, last);
        _assign(n, [&](T* dst) { _copy_range(dst, first, last); });
    }
    constexpr void assign(std::initializer_list<T> list)
    {
        _assign(list.size(), [&](T* dst) { _copy_range(dst, list.begin(), list.end()); });
    }
    constexpr void assign(size_t n, const T& v)
    {
        _assign(n, [&](T* dst) { _copy_n(dst, v, n); });
    }

    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr T* insert(T* pos, Iter first, Iter last)
    {
        size_t n = std::distance(first, last);
        return _insert(pos, n, [&](T* addr) { _copy_range(addr, first, last); });
    }
    constexpr T* insert(T* pos, std::initializer_list<T> list)
    {
        return _insert(pos, list.size(), [&](T* addr) { _copy_range(addr, list.begin(), list.end()); });
    }
    constexpr T* insert(T* pos, const T& v)
    {
        return _insert(pos, 1, [&](T* addr) { _copy_n(addr, v, 1); });
    }

    constexpr T* erase(T* first, T* last)
    {
        _copy_range(first, last, end());
        _shrink(std::distance(first, last));
        return first;
    }
    constexpr T* erase(T* pos)
    {
        return erase(pos, pos + 1);
    }


public:
    template<class Number, fc_require(std::is_integral_v<Number>)>
    inline Number to_number(size_t* idx, int base) const
    {
        const T* start = begin();
        const T* last = end();
        while (std::isspace(*start)) {
            ++start;
        }

        Number tmp;
        auto res = std::from_chars(start, last, tmp, base);
        if (res.ec == std::errc{}) {
            if (idx) {
                *idx = std::distance(begin(), res.ptr);
            }
            return tmp;
        }
        else {
            throw std::invalid_argument("to_number()");
            return {};
        }
    }

    template<class Number, fc_require(std::is_floating_point_v<Number>)>
    inline Number to_number(size_t* idx) const
    {
        const T* start = begin();
        const T* last = end();
        while (std::isspace(*start)) {
            ++start;
        }

        // note: std::from_chars with floating point types are not implementen on emscripten (as of 2023/07).
        // in that case, fallback to std::atof(). so, 0.0 is returned on error instead of throwing std::invalid_argument.
        if constexpr (is_from_chars_available_v<Number>) {
            Number tmp;
            auto res = std::from_chars(start, last, tmp);
            if (res.ec == std::errc{}) {
                if (idx) {
                    *idx = std::distance(begin(), res.ptr);
                }
                return tmp;
            }
            else {
                throw std::invalid_argument("to_number()");
                return {};
            }
        }
        else {
            return std::atof(start);
        }
    }

    constexpr size_t hash() const
    {
        return _hash(data(), size_bytes());
    }


protected:
    using super::_copy_range;
    using super::_copy_n;
    using super::_move_range;
    using super::_move_one;
    using super::_emplace_one;

    using super::_shrink;
    using super::_expand;
    using super::_resize;
    using super::_insert;
    using super::_assign;

    static inline size_t _hash(const void* _data, const size_t size)
    {
        constexpr size_t _basis = 14695981039346656037UL;
        constexpr size_t _prime = 1099511628211UL;
        auto data = (const std::byte*)_data;
        size_t v = _basis;
        for (size_t i = 0; i < size; ++i) {
            v ^= static_cast<size_t>(data[i]);
            v *= _prime;
        }
        return v;
    }
};

template<class T, class M1, class M2, class Tr>
bool operator==(const basic_string<T, M1, Tr>& l, const basic_string<T, M2, Tr>& r)
{
    return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin());
}
template<class T, class M1, class M2, class Tr>
bool operator!=(const basic_string<T, M1, Tr>& l, const basic_string<T, M2, Tr>& r)
{
    return l.size() != r.size() || !std::equal(l.begin(), l.end(), r.begin());
}
template<class T, class M1, class M2, class Tr>
bool operator<(const basic_string<T, M1, Tr>& l, const basic_string<T, M2, Tr>& r)
{
    return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
}
template<class T, class M1, class M2, class Tr>
bool operator>(const basic_string<T, M1, Tr>& l, const basic_string<T, M2, Tr>& r)
{
    return r < l;
}
template<class T, class M1, class M2, class Tr>
bool operator<=(const basic_string<T, M1, Tr>& l, const basic_string<T, M2, Tr>& r)
{
    return !(r < l);
}
template<class T, class M1, class M2, class Tr>
bool operator>=(const basic_string<T, M1, Tr>& l, const basic_string<T, M2, Tr>& r)
{
    return !(l < r);
}


using string = basic_string<char, dynamic_memory<char>, std::char_traits<char>>;
using wstring = basic_string<wchar_t, dynamic_memory<wchar_t>, std::char_traits<wchar_t>>;
using u16string = basic_string<char16_t, dynamic_memory<char16_t>, std::char_traits<char16_t>>;
using u32string = basic_string<char32_t, dynamic_memory<char32_t>, std::char_traits<char32_t>>;
#if __cpp_char8_t
using u8string = basic_string<char8_t, dynamic_memory<char8_t>, std::char_traits<char8_t>>;
#endif // __cpp_char8_t

template<size_t Capacity> using fixed_string = basic_string<char, fixed_memory<char, Capacity>, std::char_traits<char>>;
template<size_t Capacity> using fixed_wstring = basic_string<wchar_t, fixed_memory<wchar_t, Capacity>, std::char_traits<wchar_t>>;
template<size_t Capacity> using fixed_u16string = basic_string<char16_t, fixed_memory<char16_t, Capacity>, std::char_traits<char16_t>>;
template<size_t Capacity> using fixed_u32string = basic_string<char32_t, fixed_memory<char, Capacity>, std::char_traits<char>>;
#if __cpp_char8_t
template<size_t Capacity> using fixed_u8string = basic_string<char8_t, fixed_memory<char, Capacity>, std::char_traits<char8_t>>;
#endif // __cpp_char8_t

template<size_t Capacity> using sbo_string = basic_string<char, sbo_memory<char, Capacity>, std::char_traits<char>>;
template<size_t Capacity> using sbo_wstring = basic_string<wchar_t, sbo_memory<wchar_t, Capacity>, std::char_traits<wchar_t>>;
template<size_t Capacity> using sbo_u16string = basic_string<char16_t, sbo_memory<char16_t, Capacity>, std::char_traits<char16_t>>;
template<size_t Capacity> using sbo_u32string = basic_string<char32_t, sbo_memory<char32_t, Capacity>, std::char_traits<char32_t>>;
#if __cpp_char8_t
template<size_t Capacity> using sbo_u8string = basic_string<char8_t, sbo_memory<char8_t, Capacity>, std::char_traits<char8_t>>;
#endif // __cpp_char8_t

using string_view = std::basic_string_view<char, std::char_traits<char>>;
using wstring_view = std::basic_string_view<wchar_t, std::char_traits<wchar_t>>;
using u16string_view = std::basic_string_view<char16_t, std::char_traits<char16_t>>;
using u32string_view = std::basic_string_view<char32_t, std::char_traits<char32_t>>;
#if __cpp_char8_t
using u8string_view = std::basic_string_view<char8_t, std::char_traits<char8_t>>;
#endif // __cpp_char8_t

} // namespace ist


namespace std {

template<class T, class M, class Traits>
void swap(ist::basic_string<T, M, Traits>& l, ist::basic_string<T, M, Traits>& r) noexcept
{
    l.swap(r);
}

template<class T, class M, class Traits>
int stoi(ist::basic_string<T, M, Traits>& v, size_t* pos = nullptr, int base = 10) { return v.template to_number<int>(pos, base); }
template<class T, class M, class Traits>
long stol(ist::basic_string<T, M, Traits>& v, size_t* pos = nullptr, int base = 10) { return v.template to_number<long>(pos, base); }
template<class T, class M, class Traits>
long long stoll(ist::basic_string<T, M, Traits>& v, size_t* pos = nullptr, int base = 10) { return v.template to_number<long long>(pos, base); }
template<class T, class M, class Traits>
unsigned long stoul(ist::basic_string<T, M, Traits>& v, size_t* pos = nullptr, int base = 10) { return v.template to_number<unsigned long>(pos, base); }
template<class T, class M, class Traits>
unsigned long long stoull(ist::basic_string<T, M, Traits>& v, size_t* pos = nullptr, int base = 10) { return v.template to_number<unsigned long long>(pos, base); }
template<class T, class M, class Traits>
float stof(ist::basic_string<T, M, Traits>& v, size_t* pos = nullptr) { return v.template to_number<float>(pos); }
template<class T, class M, class Traits>
double stod(ist::basic_string<T, M, Traits>& v, size_t* pos = nullptr) { return v.template to_number<double>(pos); }
template<class T, class M, class Traits>
long double stold(ist::basic_string<T, M, Traits>& v, size_t* pos = nullptr) { return v.template to_number<long double>(pos); }

template<class T, class M, class Traits>
struct hash<ist::basic_string<T, M, Traits>>
{
    size_t operator()(const ist::basic_string<T, M, Traits>& r) const noexcept {
        return r.hash();
    }
};

} // namespace std
