#include "Test.h"
#include "flat_container/flat_set.h"
#include "flat_container/flat_map.h"
#include "flat_container/raw_vector.h"
#include "flat_container/vector.h"
#include "flat_container/string.h"
#include <set>
#include <map>
#include <memory>
#include <unordered_map>
#include <random>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#endif // __EMSCRIPTEN__


#if defined(_M_IX86) || defined(__i386__)
#   define fc_x84
#elif defined(_M_X64) || defined(__x86_64__)
#   define fc_x84
#   define fc_x84_64
#endif

#ifdef fc_x84
#   include <nmmintrin.h>
#endif

#ifdef __clang__
#   define fc_no_sanitize_address __attribute__((no_sanitize_address))
#elif _MSC_VER
#   define fc_no_sanitize_address __declspec(no_sanitize_address)
#endif


using test::Timer;
using string = ist::string;

testCase(test_flat_set)
{
    std::set<string> sset;
    ist::flat_set<string> fset;
    ist::fixed_set<string, 32> xset;
    ist::sbo_set<string, 8> bset;

    std::byte buf[sizeof(string) * 32];
    ist::mapped_set<string> vset(buf, 32);

    auto check = [&]() {
        testExpect(sset.size() == fset.size());
        testExpect(sset.size() == xset.size());
        testExpect(sset.size() == bset.size());
        testExpect(sset.size() == vset.size());

        auto i1 = sset.begin();
        auto i2 = fset.begin();
        auto i3 = xset.begin();
        auto i4 = bset.begin();
        auto i5 = vset.begin();
        while (i1 != sset.end()) {
            testExpect(*i1 == *i2);
            testExpect(*i1 == *i3);
            testExpect(*i1 == *i4);
            testExpect(*i1 == *i5);
            ++i1; ++i2; ++i3; ++i4; ++i5;
        }
    };
    auto insert = [&](const string& v) {
        sset.insert(v);
        fset.insert(v);
        xset.insert(v);
        bset.insert(v);
        vset.insert(v);
    };
    auto insert_il = [&](std::initializer_list<string>&& v) {
        sset.insert(v);
        fset.insert(v);
        xset.insert(v);
        bset.insert(v);
        vset.insert(v);
    };
    auto erase = [&](const string& v) {
        sset.erase(v);
        fset.erase(v);
        xset.erase(v);
        bset.erase(v);
        vset.erase(v);
    };

    string data[]{ "e", "a", "e", "b", "c", "d", "c", "b", "d", "a", };
    for (auto& v : data) {
        insert(v);
    }
    insert_il({ "abc", "def", "ghi", "jkl" });
    check();

    testExpect(fset == xset);
    testExpect(!(fset != xset));
    testExpect(!(fset < xset));
    testExpect(!(fset > xset));
    testExpect(fset <= xset);
    testExpect(fset >= xset);

    testExpect(fset == bset);
    testExpect(!(fset != bset));
    testExpect(!(fset < bset));
    testExpect(!(fset > bset));
    testExpect(fset <= bset);
    testExpect(fset >= bset);

    erase("c");
    erase("a");
    erase("x");
    check();
}


testCase(test_flat_map)
{
    std::map<string, int> smap;
    ist::flat_map<string, int> fmap;
    ist::fixed_map<string, int, 32> xmap;
    ist::sbo_map<string, int, 8> bmap;

    std::byte buf[sizeof(std::pair<string, int>) * 32];
    ist::mapped_map<string, int> vmap(buf, 32);

    auto check = [&]() {
        testExpect(smap.size() == fmap.size());
        testExpect(smap.size() == xmap.size());
        testExpect(smap.size() == bmap.size());
        testExpect(smap.size() == vmap.size());

        auto i1 = smap.begin();
        auto i2 = fmap.begin();
        auto i3 = xmap.begin();
        auto i4 = bmap.begin();
        auto i5 = vmap.begin();
        while (i1 != smap.end()) {
            testExpect(i1->first == i2->first); testExpect(i1->second == i2->second);
            testExpect(i1->first == i3->first); testExpect(i1->second == i3->second);
            testExpect(i1->first == i4->first); testExpect(i1->second == i4->second);
            testExpect(i1->first == i5->first); testExpect(i1->second == i5->second);
            ++i1; ++i2; ++i3; ++i4; ++i5;
        }
    };
    auto insert = [&](const std::pair<string, int>& v) {
        smap.insert(v);
        fmap.insert(v);
        xmap.insert(v);
        bmap.insert(v);
        vmap.insert(v);
    };
    auto insert_il = [&](std::initializer_list<std::pair<const string, int>>&& v) {
        smap.insert(v);
        fmap.insert(v);
        xmap.insert(v);
        bmap.insert(v);
        vmap.insert(v);
    };
    auto erase = [&](const string& v) {
        smap.erase(v);
        fmap.erase(v);
        xmap.erase(v);
        bmap.erase(v);
        vmap.erase(v);
    };

    std::pair<string, int> data[]{
        {"a", 10},
        {"c", 3},
        {"e", 50},
        {"d", 4},
        {"b", 20},
        {"b", 2},
        {"d", 40},
        {"e", 5},
        {"c", 30},
        {"a", 1},
    };

    for (auto& v : data) {
        insert(v);
    }
    insert_il({ {"abc", 100}, {"def", 200}, {"ghi", 300}, {"jkl", 400} });
    check();

    testExpect(fmap == xmap);
    testExpect(!(fmap != xmap));
    testExpect(!(fmap < xmap));
    testExpect(!(fmap > xmap));
    testExpect(fmap <= xmap);
    testExpect(fmap >= xmap);

    testExpect(fmap == bmap);
    testExpect(!(fmap != bmap));
    testExpect(!(fmap < bmap));
    testExpect(!(fmap > bmap));
    testExpect(fmap <= bmap);
    testExpect(fmap >= bmap);

    erase("c");
    erase("a");
    erase("x");
    check();
}


testCase(test_fixed_vector)
{
    printf("is_mapped_memory_v<ist::fixed_vector<int, 8>>: %d\n",
        (int)ist::is_mapped_memory_v<ist::fixed_vector<int, 8>>);
    printf("is_mapped_memory_v<ist::mapped_vector<int>>: %d\n",
        (int)ist::is_mapped_memory_v<ist::mapped_vector<int>>);

    {
        // basic tests
        ist::fixed_vector<string, 128> data, data2, data3;
        ist::sbo_vector<string, 32> sdata;
        ist::vector<string> ddata;

        std::byte buf[sizeof(string) * 128];
        ist::mapped_vector<string> vdata(buf, 128);

        auto make_data = [](auto& dst) {
            string tmp;
            for (int i = 0; i < 64; ++i) {
                tmp += ' ' + char(i);

                switch (i % 8) {
                case 0: dst.push_back(tmp); break;
                case 1: {
                    auto t = tmp;
                    dst.push_back(std::move(t));
                    break;
                }
                case 2: dst.emplace_back(tmp.c_str(), tmp.size()); break;
                case 3: dst.resize(dst.size() + 1, tmp); break;
                case 4: dst.insert(dst.begin(), tmp); break;
                case 5: dst.insert(dst.begin(), &tmp, &tmp + 1); break;
                case 6: dst.insert(dst.begin(), std::initializer_list<string>{tmp}); break;
                case 7: dst.erase(dst.begin() + dst.size() / 2); break;
                case 8: dst.emplace(dst.begin() + dst.size() / 2, tmp.c_str(), tmp.size()); break;
                }
            }
        };
        make_data(data);
        make_data(sdata);
        make_data(ddata);
        make_data(vdata);

        testExpect(data == sdata);
        testExpect(!(data != sdata));
        testExpect(!(data < sdata));
        testExpect(!(data > sdata));
        testExpect(data <= sdata);
        testExpect(data >= sdata);

        testExpect(data == ddata);
        testExpect(!(data != ddata));
        testExpect(!(data < ddata));
        testExpect(!(data > ddata));
        testExpect(data <= ddata);
        testExpect(data >= ddata);

        testExpect(data == vdata);
        testExpect(!(data != vdata));
        testExpect(!(data < vdata));
        testExpect(!(data > vdata));
        testExpect(data <= vdata);
        testExpect(data >= vdata);

        data.erase(data.begin() + 32, data.begin() + 40);
        testExpect(data.size() == 40);

        data2 = data;
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data2[i]);
        }

        data3 = std::move(data2);
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data3[i]);
        }

        data2.assign(data3.begin(), data3.end());
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data2[i] == data3[i]);
        }

        data3.assign(64, data2[0]);
        for (auto& v : data3) {
            testExpect(v == data2[0]);
        }
    }

    {
        // with non-copyable element
        class elem
        {
        public:
            elem(const string& v) : value(v) {}
            elem(const char* str, size_t len) : value(str, len) {}
            elem() = delete;
            elem(const elem&) = delete;
            elem(elem&&) = default;
            elem& operator=(const elem&) = delete;
            elem& operator=(elem&&) = default;
            bool operator==(const elem& v) const { return value == v.value; }

            string value;
        };

        ist::fixed_vector<elem, 128> data, data2, data3, data4;
        //std::vector<elem> data, data2, data3;

        string tmp;
        for (int i = 0; i < 64; ++i) {
            tmp += ' ' + char(i);

            switch (i % 4) {
            case 0: data.push_back(elem(tmp)); break;
            case 1: data.emplace_back(tmp.c_str(), tmp.size()); break;
            case 2: data.insert(data.begin(), elem(tmp)); break;
            case 3: data.emplace(data.begin(), tmp.c_str(), tmp.size()); break;
            }
        }

        data.erase(data.begin() + 32, data.begin() + 40);
        testExpect(data.size() == 56);

        for (size_t i = 0; i < data.size(); ++i) {
            data2.push_back(elem(data[i].value));
        }
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data2[i]);
        }

        data3 = std::move(data2);
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data3[i]);
        }

        data4.push_back(elem("abc"));
        data4.emplace_back("def");
        std::swap(data4, data3);
        testExpect(data3.size() == 2);
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data4[i]);
        }
    }
}

testCase(test_fixed_raw_vector)
{
    // causes static assertion failure
    //ist::fixed_raw_vector<string, 128> hoge;

    {
        // basic tests
        ist::fixed_raw_vector<int, 128> data, data2, data3;
        ist::sbo_raw_vector<int, 32> sdata;
        ist::raw_vector<int> ddata;

        std::byte buf1[sizeof(int) * 128];
        ist::mapped_raw_vector<int> vdata(buf1, 128);

        auto make_data = [](auto& dst){
            int tmp = 0;
            for (int i = 0; i < 64; ++i) {
                tmp += i;

                switch (i % 9) {
                case 0: dst.push_back(tmp); break;
                case 1: {
                    auto t = tmp;
                    dst.push_back(std::move(t));
                    break;
                }
                case 2: dst.emplace_back(tmp); break;
                case 3: dst.resize(dst.size() + 1, tmp); break;
                case 4: dst.insert(dst.begin(), tmp); break;
                case 5: dst.insert(dst.begin(), &tmp, &tmp + 1); break;
                case 6: dst.insert(dst.begin(), std::initializer_list<int>{tmp}); break;
                case 7: dst.erase(dst.begin() + dst.size() / 2); break;
                case 8: dst.emplace(dst.begin() + dst.size() / 2, tmp); break;
                }
            }
        };
        make_data(data);
        make_data(sdata);
        make_data(ddata);
        make_data(vdata);

        testExpect(data == sdata);
        testExpect(!(data != sdata));
        testExpect(!(data < sdata));
        testExpect(!(data > sdata));
        testExpect(data <= sdata);
        testExpect(data >= sdata);

        testExpect(data == ddata);
        testExpect(!(data != ddata));
        testExpect(!(data < ddata));
        testExpect(!(data > ddata));
        testExpect(data <= ddata);
        testExpect(data >= ddata);

        testExpect(data == vdata);
        testExpect(!(data != vdata));
        testExpect(!(data < vdata));
        testExpect(!(data > vdata));
        testExpect(data <= vdata);
        testExpect(data >= vdata);

        data.erase(data.begin() + 32, data.begin() + 40);
        testExpect(data.size() == 42);

        data2 = data;
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data2[i]);
        }

        data3 = std::move(data2);
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data3[i]);
        }

        data2.assign(data3.begin(), data3.end());
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data2[i] == data3[i]);
        }

        data3.assign(64, data2[0]);
        for (auto& v : data3) {
            testExpect(v == data2[0]);
        }

        //view = view2;
    }

}


testCase(test_constant_iterator)
{
    {
        using value_t = std::iterator_traits<ist::constant_iterator<string>>::value_type;
        printf("%d\n", std::is_same_v<value_t, string> ? 1 : 0);

        string data = "abcdefg";

        auto first = ist::make_constant_iterator(data);
        auto last = first + 4;

        for (; first != last; ++first) {
            printf("%s\n", first->c_str());
        }
    }
}


testCase(test_fixed_string)
{
    {
        ist::fixed_string<32> empty;
        ist::string sint = "  666   ";
        ist::string fint = "    666.666    ";

        printf("stoi: %d\n", std::stoi(sint));
        printf("stof: %f\n", std::stof(fint));
        printf("stod: %lf\n", std::stod(fint));

        std::unordered_map<ist::string, int> hoge{{"a", 999}};
        printf("std::unordered_map<ist::string, int>: %d\n", hoge["a"]);
    }
    {
        ist::fixed_string<32> abc = "12345";
        ist::fixed_string<32> def = "67890";

#define check(exp) testExpect(exp); printf(#exp ": %d\n", (int)(exp))
        check(!(abc == def));
        check( (abc != def));
        check( (abc <  def));
        check( (abc <= def));
        check(!(abc >  def));
        check(!(abc >= def));

        check(!(abc == def.data()));
        check( (abc != def.data()));
        check( (abc <  def.data()));
        check( (abc <= def.data()));
        check(!(abc >  def.data()));
        check(!(abc >= def.data()));

        check(!(abc.data() == def));
        check( (abc.data() != def));
        check( (abc.data() <  def));
        check( (abc.data() <= def));
        check(!(abc.data() >  def));
        check(!(abc.data() >= def));
#undef check

        testExpect(abc.starts_with('1'));
        testExpect(abc.starts_with("123"));
        testExpect(abc.starts_with(std::string_view("123")));
        testExpect(abc.ends_with('5'));
        testExpect(abc.ends_with("345"));
        testExpect(abc.ends_with(std::string_view("345")));

        abc += '?';
        testExpect(abc == "12345?");

        abc += "hoge";
        testExpect(abc == "12345?hoge");

        abc += {'a', 'b', 'c'};
        testExpect(abc == "12345?hogeabc");

        abc += std::string_view("def");
        testExpect(abc == "12345?hogeabcdef");

        auto check_pos = [&](size_t pos, char v) {
            return abc[pos] == v;
        };
        auto check_npos = [&](size_t pos) {
            return pos == ist::string::npos;
        };
        testExpect(check_pos(abc.find("345"), '3'));
        testExpect(check_npos(abc.find("678")));
        testExpect(check_pos(abc.find_first_of("abcdef?"), '?'));
        testExpect(check_npos(abc.find_first_of("xyz")));
        testExpect(check_pos(abc.find_first_not_of("12345"), '?'));
        testExpect(check_npos(abc.find_first_not_of("12345?hogeabcdef")));
        testExpect(check_pos(abc.find_last_of("?12345"), '?'));
        testExpect(check_npos(abc.find_last_of("xyz")));
        testExpect(check_pos(abc.find_last_not_of("hogeabcdef"), '?'));
        testExpect(check_npos(abc.find_last_not_of("12345?hogeabcdef")));

        std::string sabc = std::string(abc);
        testExpect(check_pos(sabc.find("345"), '3'));
        testExpect(check_npos(sabc.find("678")));
        testExpect(check_pos(sabc.find_first_of("abcdef?"), '?'));
        testExpect(check_npos(sabc.find_first_of("xyz")));
        testExpect(check_pos(sabc.find_first_not_of("12345"), '?'));
        testExpect(check_npos(sabc.find_first_not_of("12345?hogeabcdef")));
        testExpect(check_pos(sabc.find_last_of("?12345"), '?'));
        testExpect(check_npos(sabc.find_last_of("xyz")));
        testExpect(check_pos(sabc.find_last_not_of("hogeabcdef"), '?'));
        testExpect(check_npos(sabc.find_last_not_of("12345?hogeabcdef")));


        abc.replace(abc.begin() + 3, abc.begin() + 5, "6789");
        testExpect(abc == "1236789?hogeabcdef");

        abc.replace(abc.find("?"), 11, std::string_view(""));
        testExpect(abc == "1236789");

        abc = abc + 'a';
        abc = std::move(abc) + 'b';
        abc = abc + "cd";
        abc = std::move(abc) + "ef";
        abc = abc + std::string_view("gh");
        abc = std::move(abc) + std::string_view("ij");
        abc = 'z' + abc;
        abc = "xy" + abc;
        abc = std::string_view("vw") + abc;
        testExpect(abc == "vwxyz1236789abcdefghij");

        auto xyz = abc.substr(2, 3);
        testExpect(xyz == "xyz");
    }
}


#ifdef __EMSCRIPTEN__
using emscripten::val;



template <typename T> constexpr bool is_std_string = false;
template <typename... Args> constexpr bool is_std_string<std::basic_string<Args...>> = true;

template <typename T> constexpr bool is_std_unique_ptr = false;
template <typename... Args> constexpr bool is_std_unique_ptr<std::unique_ptr<Args...>> = true;

template <typename T> constexpr bool is_std_shared_ptr = false;
template <typename... Args> constexpr bool is_std_shared_ptr<std::shared_ptr<Args...>> = true;

template <typename T, typename = void>
constexpr bool is_ranged = false;
template <typename T>
constexpr bool is_ranged<T, std::void_t<decltype(std::begin(std::declval<T&>())), decltype(std::end(std::declval<T&>()))>> = true;

template<typename T>
static inline val raw_ptr(const T* p)
{
    using namespace emscripten;
    return p ?
        val::take_ownership(internal::_emval_take_value(internal::TypeID<T>::get(), &p)) :
        val::null();
}

template<class T>
static inline val make_val(const T& v)
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
    else if constexpr (is_std_unique_ptr<T> || is_std_shared_ptr<T>) {
        return raw_ptr(v.get());
    }
    else if constexpr (std::is_pointer_v<T>) {
        return raw_ptr(v);
    }
    else {
        return raw_ptr(&v);
    }
}

class Iterator;
using IteratorPtr = std::shared_ptr<Iterator>;
class Iterable;
using IterablePtr = std::shared_ptr<Iterable>;

class Iterator
{
friend class Iterable;
public:
    template<typename... Args>
    static val create(Args&&... args)
    {
        auto* r = new Iterator(std::forward<Args>(args)...);
        r->on_done_ = [r]() { delete r; };
        return raw_ptr(r);
    }

    val next()
    {
        return next_();
    }

private:
    template<class Iter>
    Iterator(Iter begin, Iter end)
    {
        static_assert(sizeof(Iter) <= sizeof(void*)); // とりえあえずポインタと同サイズのみ受け付ける
        static_assert(std::is_trivially_destructible_v<Iter>); // デストラクタの面倒は見ない

        Iter* holder = new (buf_) Iter[2]{ begin, end };
        current_ = val::object();
        current_.set("done", false);

        next_ = [this, holder]() {
            Iter& p = holder[0];
            Iter& end = holder[1];
            if (p != end) {
                current_.set("value", make_val(*p++));
                return current_;
            }
            else {
                val tmp = std::move(current_);
                tmp.set("done", true);
                on_done_();
                return tmp;
            }
        };
    }

    template<class T, std::enable_if_t<is_ranged<T>, int> = 0>
    Iterator(const T& container)
        : Iterator(std::begin(container), std::end(container))
    {
    }

private:
    std::function<val()> next_;
    std::function<void()> on_done_;
    char buf_[sizeof(void*) * 2];
    val current_;
};

class Iterable
{
public: 
    template<typename... Args>
    static val create(Args&&... args)
    {
        auto* r = new Iterable(std::forward<Args>(args)...);
        r->iter_.on_done_ = [r]() { delete r; };
        return raw_ptr(r);
    }

    val iterator()
    {
        return raw_ptr(&iter_);
    }

private:
    template<typename... Args>
    Iterable(Args&&... args)
        : iter_(std::forward<Args>(args)...)
    {
    }

    Iterator iter_;
};


class TestData
{
public:
    TestData(int v = 0) : data_(v) {}

    void test()
    {
        printf("TestData::test(): %d\n", data_);
    }

private:
    int data_ = 0;
};

class TestIterable
{
public:
    val getMembers() const
    {
        return Iterable::create(data_);
    }

private:
    TestData data_[8]{ 100, 101, 102, 103, 104, 105, 106, 107 };
};


static void __testfunc(val a) {
    using namespace emscripten;
    TestData* hogep = a.as<TestData*>(allow_raw_pointers());
    hogep->test();
};


EMSCRIPTEN_BINDINGS(test)
{
    using namespace emscripten;
    class_<Iterator>("Iterator")
        .smart_ptr<IteratorPtr>("IteratorPtr")
        .function("next", &Iterator::next)
        ;
    class_<Iterable>("Iterable")
        .smart_ptr<IterablePtr>("IterablePtr")
        .function("@@iterator", &Iterable::iterator)
        ;

    class_<TestData>("TestData")
        .constructor<int>()
        .function("test", &TestData::test)
        ;
    class_<TestIterable>("TestIterable")
        .constructor()
        .property("members", &TestIterable::getMembers)
        ;
    function("__testfunc", &__testfunc);
}

testCase(test_ems_iterable)
{
    using namespace emscripten;

    {
        TestData hoge(42);
        val hogev = raw_ptr(&hoge);
        TestData* hogep = hogev.as<TestData*>(allow_raw_pointers());
        hogep->test();
    }
    {
        val obj{ TestData(43) };
        obj.call<void>("test");
        TestData* hogep = obj.as<TestData*>(allow_raw_pointers());
        hogep->test();


        obj.set("test2", val::module_property("__testfunc"));
        obj.call<void>("test2", obj);
    }
}

#endif // __EMSCRIPTEN__
