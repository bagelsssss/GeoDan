// Unit checks for core/hitbox_model.hpp and core/hitbox_trail.hpp.
// Build: g++ -std=c++20 -Wall -Wextra -Werror -Icore tests/test_hitbox_model.cpp -o t && ./t
#include <cstdio>
#include <cstdlib>
#include <array>
#include <optional>
#include <vector>

#include "hitbox_model.hpp"
#include "hitbox_trail.hpp"

namespace hb = geodan::core::hitbox;
namespace tr = geodan::core::trail;

static int failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                  \
        }                                                                \
    } while (0)

int main() {
    // Classification: hazards, solids, modifiers, unclassified, and skipped.
    CHECK(hb::classifyType(2).category == hb::Category::Hazard && hb::classifyType(2).draw);
    CHECK(hb::classifyType(47).category == hb::Category::Hazard);
    CHECK(hb::classifyType(0).category == hb::Category::Solid);
    CHECK(hb::classifyType(25).category == hb::Category::Solid);
    CHECK(hb::classifyType(8).category == hb::Category::Modifier);   // yellow pad
    CHECK(hb::classifyType(21).category == hb::Category::Unclassified);
    CHECK(hb::classifyType(21).draw);
    CHECK(!hb::classifyType(7).draw);                               // decoration
    CHECK(!hb::classifyType(999).draw);                             // unknown

    // Recorder ignore list: pickups 22 and 30 only.
    CHECK(hb::isRecorderIgnored(22));
    CHECK(hb::isRecorderIgnored(30));
    CHECK(!hb::isRecorderIgnored(40));
    CHECK(!hb::isRecorderIgnored(2));

    // Circle: 48 points, all at the radius from the centre.
    auto circle = hb::circlePolygon({10.f, 20.f}, 5.f);
    CHECK(circle.size() == 48);
    for (const auto& p : circle) {
        const float d = std::hypot(p.x - 10.f, p.y - 20.f);
        CHECK(std::abs(d - 5.f) < 1e-3f);
    }
    CHECK(hb::circlePolygon({0.f, 0.f}, 0.f).empty());

    // Slope triangle drops exactly one corner, chosen by the flip flags.
    std::array<hb::Pt, 4> box{{{0, 0}, {1, 0}, {1, 1}, {0, 1}}};
    CHECK(hb::slopeTriangle(box, false, false).size() == 3);
    CHECK(hb::slopeExcludedCorner(false, false) == 3);
    CHECK(hb::slopeExcludedCorner(true, true) == 1);

    // Longest edge of a right triangle is the hypotenuse (index 1: (1,0)->(0,1)).
    std::vector<hb::Pt> tri{{0, 0}, {2, 0}, {0, 2}};
    CHECK(hb::longestEdge(tri) == 1);

    // Trail: samples are added per tick, the trail is capped in time, and
    // missing players add nothing.
    tr::Tracker t;
    tr::Snapshot s{{0, 0, 1, 1}, std::nullopt};
    for (int i = 0; i < 10; ++i) t.observeTick(0.1f, s, std::nullopt);
    CHECK(t.trail(0).size() == 10);
    CHECK(t.trail(1).empty());

    t.setTrailSeconds(0.5);
    for (int i = 0; i < 50; ++i) t.observeTick(0.1f, s, std::nullopt);
    // 0.5 s at 0.1 s per tick: about 6 samples kept (the window edge plus the newest).
    CHECK(t.trail(0).size() <= 7);
    CHECK(t.trail(0).size() >= 5);

    // Bad dt is ignored, so time does not move backwards or jump.
    const double before = t.trail(0).back().elapsedSeconds;
    t.observeTick(-1.f, s, std::nullopt);
    CHECK(t.trail(0).back().elapsedSeconds == before);

    t.reset();
    CHECK(t.trail(0).empty());

    if (failures == 0) std::puts("hitbox model + trail: all checks passed");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
