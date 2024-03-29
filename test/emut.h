#pragma once
#ifdef __EMSCRIPTEN__
#include <typeinfo>
#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>

namespace emut {

using emscripten::val;

template <typename T> constexpr bool is_std_string = false;
template <typename... Args> constexpr bool is_std_string<std::basic_string<Args...>> = true;

template <typename T> constexpr bool is_std_unique_ptr = false;
template <typename... Args> constexpr bool is_std_unique_ptr<std::unique_ptr<Args...>> = true;

template <typename T> constexpr bool is_std_shared_ptr = false;
template <typename... Args> constexpr bool is_std_shared_ptr<std::shared_ptr<Args...>> = true;


template<typename T>
inline val make_pointer(const T* p)
{
    using namespace emscripten;
    return p ?
        val::take_ownership(internal::_emval_take_value(internal::TypeID<T>::get(), &p)) :
        val::null();
}
template<typename T>
inline val make_pointer(const std::shared_ptr<T>& p)
{
    return make_pointer(p.get());
}
template<typename T>
inline val make_pointer(const std::unique_ptr<T>& p)
{
    return make_pointer(p.get());
}

template<class T>
inline val make_val(const T& v)
{
    if constexpr (std::is_arithmetic_v<T>) {
        if constexpr (std::is_integral_v<T> && sizeof(T) >= 8) {
            // 64 bit int require explicit conversion
            return val((double)v);
        }
        else {
            return val(v);
        }
    }
    else if constexpr (is_std_string<T>) {
        return val(v);
    }
    else if constexpr (is_std_unique_ptr<T> || is_std_shared_ptr<T> || std::is_pointer_v<T>) {
        return make_pointer(v);
    }
    else {
        return make_pointer(&v);
    }
}

template<class Iter>
class Iterator
{
public:
    Iterator(Iter begin, Iter end) :
        begin_(begin), end_(end)
    {
        static struct binder {
            binder() {
                emscripten::class_<Iterator>(typeid(Iterator).name())
                    .function("next", &Iterator::next)
                    ;
            }
        } bind;

        current_ = val::object();
        current_.set("done", false);
    }

    val next()
    {
        if (begin_ != end_) {
            current_.set("value", make_val(*begin_++));
            return current_;
        }
        else {
            // move current_ before on_done_() destroys this.
            val tmp = std::move(current_);
            tmp.set("done", true);
            on_done_();
            return tmp;
        }
    }

    template<class Func>
    void setOnDone(Func&& f)
    {
        on_done_ = std::move(f);
    }

private:
    Iter begin_, end_;
    std::function<void()> on_done_;
    val current_;
};

template<class Iter>
class Iterable
{
public:
    Iterable(Iter begin, Iter end)
        : iter_(begin, end)
    {
        static struct binder {
            binder() {
                emscripten::class_<Iterable>(typeid(Iterable).name())
                    .function("@@iterator", &Iterable::iterator)
                    ;
            }
        } bind;
    }

    val iterator()
    {
        return make_pointer(&iter_);
    }

    template<class Func>
    void setOnDone(Func&& f)
    {
        iter_.setOnDone(std::move(f));
    }

private:
    Iterator<Iter> iter_;
};


template <typename T, typename = void>
constexpr bool is_ranged = false;
template <typename T>
constexpr bool is_ranged<T, std::void_t<decltype(std::begin(std::declval<T&>())), decltype(std::end(std::declval<T&>()))>> = true;

template<class Iter>
inline val make_iterator(Iter begin, Iter end)
{
    auto* r = new Iterator<Iter>(begin, end);
    r->setOnDone([r]() { delete r; });
    return make_pointer(r);
}

template<class Container, std::enable_if_t<is_ranged<Container>, int> = 0>
inline val make_iterator(const Container& cont)
{
    return make_iterator(std::begin(cont), std::end(cont));
}


template<class Iter>
inline val make_iterable(Iter begin, Iter end)
{
    auto* r = new Iterable<Iter>(begin, end);
    r->setOnDone([r]() { delete r; });
    return make_pointer(r);
}

template<class Container, std::enable_if_t<is_ranged<Container>, int> = 0>
inline val make_iterable(const Container& cont)
{
    return make_iterable(std::begin(cont), std::end(cont));
}


template<class Func>
inline val make_function(Func&& func)
{
    static struct binder {
        binder() {
            emscripten::class_<Func>(typeid(Func).name())
                // JavaScript doesn't have operator overloading. define call() in place of it.
                .function("call", &Func::operator())
                ;
        }
    } bind;
    return val(std::move(func));
}

} // namespace emut
#endif // __EMSCRIPTEN__
