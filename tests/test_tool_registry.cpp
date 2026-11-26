// Run: g++ -std=c++20 -Wall -Wextra -Werror -Icore tests/test_tool_registry.cpp -o /tmp/tr && /tmp/tr
#include <cstdio>
#include <string_view>
#include "tool_registry.hpp"

using namespace geodan::core;

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

int main() {
    CHECK(kToolCount == 5);
    std::size_t availableCount = 0;
    for (const auto& tool : kTools)
        if (canEnable(tool)) ++availableCount;
    CHECK(availableCount == 3);

    for (std::size_t i = 0; i < kToolCount; ++i) {
        const auto& t = kTools[i];
        CHECK(!t.id.empty() && !t.title.empty() && !t.blurb.empty());
        for (std::size_t j = 0; j < i; ++j) {
            CHECK(kTools[j].id != t.id);        // ids unique
            CHECK(kTools[j].title != t.title);  // titles unique
        }
        CHECK(canEnable(t) == (t.status == Status::Available));
    }

    // Saved values win over defaults, for available tools only.
    auto nothingSaved = [](std::string_view, bool fallback) { return fallback; };
    auto everythingOn = [](std::string_view, bool) { return true; };
    CHECK(countOn(nothingSaved) == 0);
    CHECK(countOn(everythingOn) == 3);   // only the three available tools can be enabled
    for (const auto& t : kTools)
        if (t.status == Status::Planned) CHECK(!isOn(t, everythingOn));

    if (g_fail) { std::printf("%d failure(s)\n", g_fail); return 1; }
    std::printf("tool registry: all checks passed\n");
    return 0;
}
