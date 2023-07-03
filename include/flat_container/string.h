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

template<typename Container, class Char, typename = void>
constexpr bool is_string_like_v = false;
template<typename Container, class Char>
constexpr bool is_string_like_v<Container, Char, std::enable_if_t<
    std::is_same_v<remove_const_pointer_t<decltype(std::declval<Container>().data())>, Char>>> = true;


template<class T, class Memory, class Traits = std::char_traits<T>>
class basic_string : public Memory
{
using super = Memory;
public:
    using traits_type = Traits;

    static const size_t npos = ~0;

    constexpr basic_string()
    {
        if (capacity() != 0) {
            _null_terminate();
        }
    }
    constexpr basic_string(const basic_string& r) = default;
    constexpr basic_string(basic_string&& r) noexcept = default;
    constexpr basic_string& operator=(const basic_string& r) = default;
    constexpr basic_string& operator=(basic_string&& r) noexcept = default;

    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr basic_string(const T* v) { assign(v); }
    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr basic_string(const T* v, size_t n) { assign(v, n); }

    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr basic_string(std::initializer_list<T> r) { assign(r); }

    template<class Iter, bool view = is_memory_view_v<super>, fc_require(!view), fc_require(is_iterator_v<Iter>)>
    constexpr basic_string(Iter first, Iter last) { assign(first, last); }

    template<class String, bool view = is_memory_view_v<super>, fc_require(!view), fc_require(is_string_like_v<String, T>)>
    constexpr basic_string(const String& str) { assign(str); }

    template<bool view = is_memory_view_v<super>, fc_require(view)>
    constexpr basic_string(void* data, size_t capacity, size_t size = 0)
        : super(data, capacity, size)
        , basic_string()
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

    constexpr const T* c_str() const { return data(); }
    constexpr size_t length() const { return size(); }

    constexpr void resize(size_t n)
    {
        _resize(n, [&](T*) {}); // new elements are uninitialized
        _null_terminate();
    }
    constexpr void resize(size_t n, T v)
    {
        _resize(n, [&](T* addr) { *addr = v; });
        _null_terminate();
    }

    constexpr void push_back(const T& v)
    {
        _expand(1, [&](T* addr) { *addr = v; });
        _null_terminate();
    }

    constexpr void pop_back()
    {
        _shrink(1);
        _null_terminate();
    }

    constexpr basic_string& assign(const T* str, size_t n)
    {
        _assign(n, [&](T* dst) { _copy_range(dst, str, str + n); });
        _null_terminate();
        return *this;
    }
    constexpr basic_string& assign(const T* str)
    {
        return assign(str, Traits::length(str));
    }
    constexpr basic_string& assign(size_t n, T ch)
    {
        _assign(n, [&](T* dst) { _copy_n(dst, ch, n); });
        _null_terminate();
        return *this;
    }
    constexpr basic_string& assign(std::initializer_list<T> list)
    {
        _assign(list.size(), [&](T* dst) { _copy_range(dst, list.begin(), list.end()); });
        _null_terminate();
        return *this;
    }
    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr basic_string& assign(Iter first, Iter last)
    {
        size_t n = std::distance(first, last);
        _assign(n, [&](T* dst) { _copy_range(dst, first, last); });
        _null_terminate();
        return *this;
    }
    template<class String, fc_require(is_string_like_v<String, T>)>
    constexpr basic_string& assign(const String& str, size_t offset, size_t count = npos)
    {
        size_t n = count == npos ? str.size() - offset : count;
        auto first = str.begin() + offset;
        auto last = str.begin() + offset + n;
        return assign(first, last);
    }
    template<class String, fc_require(is_string_like_v<String, T>)>
    constexpr basic_string& assign(const String& str)
    {
        return assign(str, 0, npos);
    }

    constexpr T* insert(T* pos, T* str, size_t n)
    {
        auto r = _insert(pos, n, [&](T* addr) { _copy_range(addr, str, str + n); });
        _null_terminate();
        return r;
    }
    constexpr T* insert(T* pos, T* str)
    {
        return insert(pos, str, Traits::length(str));
    }
    constexpr T* insert(T* pos, T ch)
    {
        return insert(pos, &ch, 1);
    }
    constexpr T* insert(T* pos, size_t n, T v)
    {
        auto r = _insert(pos, n, [&](T* addr) { _copy_n(addr, v, n); });
        _null_terminate();
        return r;
    }
    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr T* insert(T* pos, Iter first, Iter last)
    {
        size_t n = std::distance(first, last);
        auto r = _insert(pos, n, [&](T* addr) { _copy_range(addr, first, last); });
        _null_terminate();
        return r;
    }
    constexpr T* insert(T* pos, std::initializer_list<T> list)
    {
        auto r = _insert(pos, list.size(), [&](T* addr) { _copy_range(addr, list.begin(), list.end()); });
        _null_terminate();
        return r;
    }

    constexpr basic_string& insert(size_t pos, const T* str, size_t offset, size_t count = npos)
    {
        size_t n = count == npos ? Traits::length(str) - offset : count;
        auto first = str;
        auto last = str + n;
        _insert(begin() + pos, n, [&](T* addr) { _copy_range(addr, first, last); });
        _null_terminate();
        return *this;
    }
    constexpr basic_string& insert(size_t pos, const T* str)
    {
        return insert(pos, str, 0, Traits::length(str));
    }
    constexpr basic_string& insert(size_t pos, size_t count, T ch)
    {
        return insert(pos, &ch, 0, 1);
    }
    template<class String, fc_require(is_string_like_v<String, T>)>
    constexpr basic_string& insert(size_t pos, const String& str, size_t offset, size_t count = npos)
    {
        size_t n = count == npos ? str.size() - offset : count;
        auto first = str.begin() + offset;
        auto last = str.begin() + offset + n;
        insert(begin() + pos, first, last);
        return *this;
    }
    template<class String, fc_require(is_string_like_v<String, T>)>
    constexpr basic_string& insert(size_t pos, const String& str)
    {
        return insert(pos, str, 0, npos);
    }

    constexpr basic_string& append(const T* str, size_t count)
    {
        return insert(size(), str, 0, count);
    }
    constexpr basic_string& append(const T* str)
    {
        return insert(size(), str);
    }
    constexpr basic_string& append(T ch)
    {
        return insert(size(), 1, ch);
    }
    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr basic_string& append(Iter first, Iter last)
    {
        insert(end(), first, last);
        return *this;
    }
    constexpr basic_string& append(std::initializer_list<T> list)
    {
        insert(end(), list.begin(), list.end());
        return *this;
    }
    template<class String, fc_require(is_string_like_v<String, T>)>
    constexpr basic_string& append(const String& str)
    {
        return insert(size(), str);
    }
    template<class String, fc_require(is_string_like_v<String, T>)>
    constexpr basic_string& append(const String& str, size_t offset, size_t count = npos)
    {
        return insert(size(), str, offset, count);
    }

    constexpr basic_string& operator+=(T ch) { return append(ch); }
    constexpr basic_string& operator+=(const T* ch) { return append(ch); }
    constexpr basic_string& operator+=(std::initializer_list<T> il) { return append(il); }
    template<class String, fc_require(is_string_like_v<String, T>)>
    constexpr basic_string& operator+=(const String& str) { return append(str); }

    constexpr T* erase(T* first, T* last)
    {
        _copy_range(first, last, end());
        _shrink(std::distance(first, last));
        _null_terminate();
        return first;
    }
    constexpr T* erase(T* pos)
    {
        return erase(pos, pos + 1);
    }
    constexpr basic_string& erase(size_t offset = 0, size_t count = npos)
    {
        size_t n = count == npos ? size() - offset : count;
        auto first = begin() + offset;
        auto last = begin() + offset + n;
        erase(first, last);
        return *this;
    }

    constexpr size_t find(const T* s, size_t offset, size_t count) const noexcept
    {
        return _find(s, count, offset);
    }
    constexpr size_t find(const T* s, size_t offset = 0) const noexcept
    {
        return _find(s, Traits::length(s), offset);
    }
    constexpr size_t find(T ch, size_t offset) const noexcept
    {
        return _find_ch(ch, offset);
    }
    template<class String, fc_require(is_string_like_v<String, T>)>
    constexpr size_t find(const String& str, size_t offset = 0) const noexcept
    {
        return _find(str.data(), str.size(), offset);
    }

    constexpr bool starts_with(T ch) const noexcept
    {
        return size() >= 1 && Traits::compare(data(), &ch, 1) == 0;
    }
    constexpr bool starts_with(const T* str) const noexcept
    {
        size_t n = Traits::length(str);
        return size() >= n && Traits::compare(data(), str, n) == 0;
    }
    template<class String, fc_require(is_string_like_v<String, T>)>
    constexpr bool starts_with(const String& str) const noexcept
    {
        size_t n = str.size();
        return size() >= n && Traits::compare(data(), str.data(), n) == 0;
    }

    constexpr bool ends_with(T ch) const noexcept
    {
        return size() >= 1 && Traits::compare(data() + size() - 1, &ch, 1) == 0;
    }
    constexpr bool ends_with(const T* str) const noexcept
    {
        size_t n = Traits::length(str);
        return size() >= n && Traits::compare(data() + size() - n, str, n) == 0;
    }
    template<class String, fc_require(is_string_like_v<String, T>)>
    constexpr bool ends_with(const String& str) const noexcept
    {
        size_t n = str.size();
        return size() >= n && Traits::compare(data() + size() - n, str.data(), n) == 0;
    }

    constexpr operator std::basic_string_view<T, Traits>() const noexcept { return { data(), size() }; }

public:
    template<class Number, fc_require(std::is_integral_v<Number>)>
    inline Number to_number(size_t* idx, int base) const
    {
        auto start = begin();
        auto last = end();
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
        auto start = begin();
        auto last = end();
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

    constexpr void _null_terminate()
    {
        reserve(size() + 1);
        data()[size()] = 0;
    }

    constexpr size_t _find(const T* str2, size_t size2, size_t offset) const noexcept
    {
        const T* str1 = data();
        const size_t size1 = size();
        if (size2 > size1 || offset > size1 - size2) {
            return static_cast<size_t>(-1);
        }
        if (size2 == 0) {
            return offset;
        }

        const auto end = str1 + (size1 - size2) + 1;
        for (auto s = str1 + offset;; ++s) {
            s = Traits::find(s, static_cast<size_t>(end - s), *str2);
            if (!s) {
                return static_cast<size_t>(-1);
            }
            if (Traits::compare(s, str2, size2) == 0) {
                return static_cast<size_t>(s - str1);
            }
        }
    }

    constexpr size_t _find_ch(T ch, size_t offset) const noexcept
    {
        const T* str1 = data();
        const size_t size1 = size();
        if (offset < size1) {
            const auto found = Traits::find(str1 + offset, size1 - offset, ch);
            if (found) {
                return static_cast<size_t>(found - str1);
            }
        }
        return static_cast<size_t>(-1);
    }

    static inline size_t _hash(const void* _data, size_t size)
    {
        constexpr size_t _basis = size_t(14695981039346656037U);
        constexpr size_t _prime = size_t(1099511628211U);
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
    return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin(), &Tr::eq);
}
template<class T, class M1, class M2, class Tr>
bool operator!=(const basic_string<T, M1, Tr>& l, const basic_string<T, M2, Tr>& r)
{
    return l.size() != r.size() || !std::equal(l.begin(), l.end(), r.begin(), &Tr::eq);
}
template<class T, class M1, class M2, class Tr>
bool operator<(const basic_string<T, M1, Tr>& l, const basic_string<T, M2, Tr>& r)
{
    return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end(), &Tr::lt);
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
