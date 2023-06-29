#pragma once
#include <string>
#include <string_view>
#include "foundation.h"

namespace ist {

template<class T, class Memory, class Traits = std::char_traits<T>>
class basic_string : public Memory
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

private:
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

} // namespace ist
