#include "Test.h"
#include "flat_container/flat_set.h"
#include "flat_container/flat_map.h"
#include <set>
#include <map>


testCase(test_flat_set)
{
    std::set<std::string> sset;
    flat_set<std::string> fset;

    std::string data[]{
        "c",
        "d",
        "b",
        "e",
        "a",
    };

    for (auto& d : data) {
        sset.insert(d);
        fset.insert(d);
    }

    {
        auto i1 = sset.begin();
        auto i2 = fset.begin();
        while (i1 != sset.end()) {
            testExpect(*i1 == *i2);
            ++i1;
            ++i2;
        }
    }

    testExpect(*fset.find("b") == "b");
}


testCase(test_flat_map)
{
    std::map<std::string, int> sset;
    flat_map<std::string, int> fset;

    std::pair<std::string, int> data[]{
        {"c", 2},
        {"d", 3},
        {"b", 1},
        {"e", 4},
        {"a", 0},
    };

    for (auto& d : data) {
        sset[d.first] = fset[d.first] = d.second;
    }

    {
        auto i1 = sset.begin();
        auto i2 = fset.begin();
        while (i1 != sset.end()) {
            testExpect(i1->first == i2->first && i1->second == i2->second);
            ++i1;
            ++i2;
        }
    }

    testExpect(fset.find("b")->second == 1);
    testExpect(fset["c"] == 2);
}
