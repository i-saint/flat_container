#include "Test.h"
#include "flat_container/flat_set.h"
#include "flat_container/flat_map.h"
#include "flat_container/fixed_vector.h"
#include <set>
#include <map>

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
            ++i1;
            ++i2;
            ++i3;
            ++i4;
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
            ++i1;
            ++i2;
            ++i3;
            ++i4;
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
    ist::fixed_vector<std::string, 128> data, data2, data3;

    std::string tmp;
    for (int i = 0; i < 128; ++i) {
        tmp += ' ' + i % 90;

        switch (i % 7) {
        case 0: data.push_back(tmp); break;
        case 1: {
            auto t = tmp;
            data.push_back(std::move(t));
            break;
        }
        case 2: data.emplace_back(tmp); break;
        case 3: data.resize(data.size() + 1, tmp); break;
        case 4: data.insert(data.end(), tmp); break;
        case 5: data.insert(data.end(), &tmp, &tmp + 1); break;
        case 6: data.insert(data.end(), std::initializer_list<std::string>{tmp}); break;
        }
    }

    data.erase(data.begin() + 100, data.begin() + 108);
    testExpect(data.size() == 120);

    data2 = data;
    for (size_t i = 0; i < data.size(); ++i) {
        testExpect(data[i] == data2[i]);
    }

    data3 = std::move(data2);
    for (size_t i = 0; i < data.size(); ++i) {
        testExpect(data[i] == data3[i]);
    }

    printf("is_memory_view_v<fixed_vector<int, 1>>: %d\n",
        ist::is_memory_view_v<ist::fixed_vector<int, 1>> ? 1 : 0);
    printf("is_memory_view_v<vector_view<int>>: %d\n",
        ist::is_memory_view_v<ist::vector_view<int>> ? 1 : 0);

    printf("vector_view<int>::is_memory_view: %d\n",
        (int)ist::vector_view<int>::is_memory_view);
}
