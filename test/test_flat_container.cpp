#include "Test.h"
#include "flat_container/flat_set.h"
#include "flat_container/flat_map.h"
#include "flat_container/fixed_vector.h"
#include "flat_container/fixed_string.h"
#include <set>
#include <map>
#include <memory>

#ifdef _WIN32
#   include <nmmintrin.h>
#endif

using test::Timer;

testCase(test_flat_set)
{
    std::set<std::string> sset;
    ist::flat_set<std::string> fset;
    ist::fixed_set<std::string, 32> xset;

    std::byte buf[sizeof(std::string) * 32];
    ist::set_view<std::string> vset(buf, 32);

    auto check = [&]() {
        testExpect(sset.size() == fset.size());
        testExpect(sset.size() == xset.size());
        testExpect(sset.size() == vset.size());

        auto i1 = sset.begin();
        auto i2 = fset.begin();
        auto i3 = xset.begin();
        auto i4 = vset.begin();
        while (i1 != sset.end()) {
            testExpect(*i1 == *i2);
            testExpect(*i1 == *i3);
            testExpect(*i1 == *i4);
            ++i1; ++i2; ++i3; ++i4;
        }
    };
    auto insert = [&](const std::string& v) {
        sset.insert(v);
        fset.insert(v);
        xset.insert(v);
        vset.insert(v);
    };
    auto insert_il = [&](std::initializer_list<std::string>&& v) {
        sset.insert(v);
        fset.insert(v);
        xset.insert(v);
        vset.insert(v);
    };
    auto erase = [&](const std::string& v) {
        sset.erase(v);
        fset.erase(v);
        xset.erase(v);
        vset.erase(v);
    };

    std::string data[]{
        "e",
        "a",
        "e",
        "b",
        "c",
        "d",
        "c",
        "b",
        "d",
        "a",
    };
    for (auto& v : data) {
        insert(v);
    }
    insert_il({ "abc", "def" });
    check();

    erase("c");
    erase("a");
    erase("x");
    check();
}


testCase(test_flat_map)
{
    std::map<std::string, int> smap;
    ist::flat_map<std::string, int> fmap;
    ist::fixed_map<std::string, int, 32> xmap;

    std::byte buf[sizeof(std::pair<std::string, int>) * 32];
    ist::map_view<std::string, int> vmap(buf, 32);

    auto check = [&]() {
        testExpect(smap.size() == fmap.size());
        testExpect(smap.size() == xmap.size());
        testExpect(smap.size() == vmap.size());

        auto i1 = smap.begin();
        auto i2 = fmap.begin();
        auto i3 = xmap.begin();
        auto i4 = vmap.begin();
        while (i1 != smap.end()) {
            testExpect(i1->first == i2->first); testExpect(i1->second == i2->second);
            testExpect(i1->first == i3->first); testExpect(i1->second == i3->second);
            testExpect(i1->first == i4->first); testExpect(i1->second == i4->second);
            ++i1; ++i2; ++i3; ++i4;
        }
    };
    auto insert = [&](const std::pair<std::string, int>& v) {
        smap.insert(v);
        fmap.insert(v);
        xmap.insert(v);
        vmap.insert(v);
    };
    auto insert_il = [&](std::initializer_list<std::pair<const std::string, int>>&& v) {
        smap.insert(v);
        fmap.insert(v);
        xmap.insert(v);
        vmap.insert(v);
    };
    auto erase = [&](const std::string& v) {
        smap.erase(v);
        fmap.erase(v);
        xmap.erase(v);
        vmap.erase(v);
    };

    std::pair<std::string, int> data[]{
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
    insert_il({ {"abc", 1000}, {"def", 500} });
    check();

    erase("c");
    erase("a");
    erase("x");
    check();
}


testCase(test_fixed_vector)
{
    printf("is_memory_view_v<ist::fixed_vector<int, 8>>: %d\n",
        (int)ist::is_memory_view_v<ist::fixed_vector<int, 8>>);
    printf("is_memory_view_v<ist::vector_view<int>>: %d\n",
        (int)ist::is_memory_view_v<ist::vector_view<int>>);

    printf("is_trivially_swappable_v<ist::fixed_vector<int, 8>>: %d\n",
        (int)ist::is_trivially_swappable_v<ist::fixed_vector<int, 8>>);
    printf("is_trivially_swappable_v<ist::vector_view<int>>: %d\n",
        (int)ist::is_trivially_swappable_v<ist::vector_view<int>>);

    {
        // basic tests
        ist::fixed_vector<std::string, 128> data, data2, data3;

        std::byte buf1[sizeof(std::string) * 128];
        std::byte buf2[sizeof(std::string) * 128];
        ist::vector_view<std::string> view(buf1, 128), view2(buf2, 128);

        std::string tmp;
        for (int i = 0; i < 64; ++i) {
            tmp += ' ' + char(i);

            switch (i % 7) {
            case 0: data.push_back(tmp); break;
            case 1: {
                auto t = tmp;
                data.push_back(std::move(t));
                break;
            }
            case 2: data.emplace_back(tmp); break;
            case 3: data.resize(data.size() + 1, tmp); break;
            case 4: data.insert(data.begin(), tmp); break;
            case 5: data.insert(data.begin(), &tmp, &tmp + 1); break;
            case 6: data.insert(data.begin(), std::initializer_list<std::string>{tmp}); break;
            }
        }

        data.erase(data.begin() + 32, data.begin() + 40);
        testExpect(data.size() == 56);

        data2 = data;
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data2[i]);
        }

        data3 = std::move(data2);
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data3[i]);
        }

        //view = view2;
    }

    {
        // with non-copyable element
        class elem
        {
        public:
            elem(const std::string& v) : value(v) {}
            elem() = delete;
            elem(const elem&) = delete;
            elem(elem&&) = default;
            elem& operator=(const elem&) = delete;
            elem& operator=(elem&&) = default;
            bool operator==(const elem& v) const { return value == v.value; }

            std::string value;
        };

        ist::fixed_vector<elem, 128> data, data2, data3, data4;
        //std::vector<elem> data, data2, data3;

        std::string tmp;
        for (int i = 0; i < 64; ++i) {
            tmp += ' ' + char(i);

            switch (i % 3) {
            case 0: data.push_back(elem(tmp)); break;
            case 1: data.emplace_back(tmp); break;
            case 2: data.insert(data.begin(), elem(tmp)); break;
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
        data4.push_back(elem("def"));
        data4.swap(data3);
        testExpect(data3.size() == 2);
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data4[i]);
        }
    }
}


#ifdef _WIN32

__declspec(noinline)
bool streq_strcmp(const char* a, const char* b, size_t len)
{
    return std::strcmp(a, b) == 0;
}

__declspec(noinline)
bool streq_strncmp(const char* a, const char* b, size_t len)
{
    return std::strncmp(a, b, len) == 0;
}

__declspec(noinline)
bool streq_memcmp(const char* a, const char* b, size_t len)
{
    return memcmp(a, b, len) == 0;
}

// len ‚Í 8 ‚Ì”{”‚Å‚ ‚é‚±‚Æ‚ª‘O’ñ
__declspec(noinline)
bool streq_uint64(const char* _a, const char* _b, size_t len)
{
    auto a = (const uint64_t*)_a;
    auto b = (const uint64_t*)_b;
    size_t n = len / 8;
    for (size_t i = 0; i < n; ++i) {
        if (*a++ != *b++) {
            return false;
        }
    }
    return true;
}

// len ‚Í 16 ‚Ì”{”‚Å‚ ‚é‚±‚Æ‚ª‘O’ñ
__declspec(noinline)
bool streq_sse42(const char* _a, const char* _b, size_t len)
{
    auto a = (const __m128i*)_a;
    auto b = (const __m128i*)_b;
    size_t n = len / 16;
    for (size_t i = 0; i < n; ++i) {
        int r = _mm_cmpestri(
            _mm_loadu_si128(a++), 16,
            _mm_loadu_si128(b++), 16,
            _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ORDERED);
        //printf("%d\n", r);
        if (r != 0) {
            return false;
        }
    }
    return true;
}
#endif

testCase(test_fixed_string)
{
#ifdef _WIN32
    const size_t num = 10000000;
    //const size_t num = 500000;
    const size_t len = 128;

    std::vector<std::string> a;
    std::vector<std::string> b;
    a.resize(num);
    b.resize(num);
    for (size_t i = 0; i < num; ++i) {
        a[i].resize(len, 0x40 + (i % 0x40));
        b[i].resize(len, 0x80 - (i % 0x40));
    }

    printf("loop count: %zu\n", num);
    printf("string length: %zu\n", len);
    {
        Timer timer;
        int r = 0;
        for (uint64_t i = 0; i < num; ++i) {
            if (streq_strncmp(a[i].c_str(), b[i].c_str(), len)) {
                ++r;
            }
        }
        printf("streq_strncmp(): %.2lfms %d\n", timer.elapsed_ms(), r);
    }
    {
        Timer timer;
        int r = 0;
        for (uint64_t i = 0; i < num; ++i) {
            if (streq_strcmp(a[i].c_str(), b[i].c_str(), len)) {
                ++r;
            }
        }
        printf("streq_strcmp(): %.2lfms %d\n", timer.elapsed_ms(), r);
    }
    {
        Timer timer;
        int r = 0;
        for (uint64_t i = 0; i < num; ++i) {
            if (streq_memcmp(a[i].c_str(), b[i].c_str(), len)) {
                ++r;
            }
        }
        printf("streq_memcmp(): %.2lfms %d\n", timer.elapsed_ms(), r);
    }
    {
        Timer timer;
        int r = 0;
        for (uint64_t i = 0; i < num; ++i) {
            if (streq_uint64(a[i].c_str(), b[i].c_str(), len)) {
                ++r;
            }
        }
        printf("streq_uint64(): %.2lfms %d\n", timer.elapsed_ms(), r);
    }
    {
        Timer timer;
        int r = 0;
        for (uint64_t i = 0; i < num; ++i) {
            if (streq_sse42(a[i].c_str(), b[i].c_str(), len)) {
                ++r;
            }
        }
        printf("streq_sse42(): %.2lfms %d\n", timer.elapsed_ms(), r);
    }
#endif
}