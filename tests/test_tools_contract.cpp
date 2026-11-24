// Checks the Tools contract without Geode. Run: g++ -std=c++20 -Wall -Wextra -Werror -Ishared tests/test_tools_contract.cpp
#include <cstdio>
#include <string_view>
#include "tools_contract.hpp"

using namespace geodan::tools;

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

int main() {
    CHECK(kModID == "local.geodan-tools");
    CHECK(kToggleCount == 6);
    for (std::size_t i = 0; i < kToggleCount; ++i) {
        const auto& t = kToggles[i];
        CHECK(!t.key.empty() && !t.label.empty());
        for (std::size_t j = 0; j < i; ++j) {
            CHECK(kToggles[j].key != t.key);      // unique setting keys
            CHECK(kToggles[j].label != t.label);  // unique labels
        }
        CHECK(canEnable(t) == t.implemented);
    }
    if (g_fail) { std::printf("%d failure(s)\n", g_fail); return 1; }
    std::printf("tools contract: all checks passed\n");
    return 0;
}
