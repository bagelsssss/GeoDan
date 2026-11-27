// GeoDan Tools - hitbox model. Pure C++ (no Geode, no Cocos) so the rules
// here can be unit-tested off-game. Ported from the old repo's
// src/hitbox_shapes.hpp, keeping only the parts the overlay uses.
#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace geodan::core::hitbox {

struct Pt {
    float x = 0.f;
    float y = 0.f;
};

// Categories decide the outline colour. Unknown interactive objects are their
// own category and are never guessed as hazards or solids.
enum class Category { Hazard, Solid, Modifier, Unclassified };

struct Classification {
    bool draw = false;
    Category category = Category::Unclassified;
};

// Pickups the recorder leaves out of the unsupported rule (they are not hazards).
// Each is still listed in unsupported_reasons.csv as an "ignored_type" note, which
// does not change the frame's status. Add a type here to stop it marking frames
// unsupported. Numbers from the 2.2081 bindings: 22 SecretCoin, 30 Collectible.
inline constexpr std::array<int, 2> kRecorderIgnoredTypes = {22, 30};

inline bool isRecorderIgnored(int type) {
    for (const int ignored : kRecorderIgnoredTypes)
        if (ignored == type) return true;
    return false;
}

// Classification by GameObjectType value (numbers from the 2.2081 bindings).
inline Classification classifyType(int type) {
    switch (type) {
        case 2:   // Hazard
        case 47:  // AnimatedHazard
            return {true, Category::Hazard};
        case 0:   // Solid
        case 25:  // Slope
            return {true, Category::Solid};
        case 3: case 4: case 5: case 6:              // gravity/ship/cube portals
        case 8: case 9: case 10:                     // pads
        case 11: case 12: case 13:                   // rings
        case 14: case 15: case 16: case 17: case 18: // mirror/ball/size portals
        case 19: case 20: case 23: case 24:          // ufo portal, modifier, dual/solo
        case 26: case 27: case 28: case 29:          // wave/robot/teleport/green ring
        case 32: case 33: case 34: case 35: case 36: // drop ring, spider, red pad/ring, custom
        case 37: case 38: case 41: case 42:          // dash rings, swing, gravity toggle
        case 43: case 44: case 46:                   // spider orb/pad, teleport orb
            return {true, Category::Modifier};
        // Interactive but not yet classified: drawn only with the unclassified setting.
        case 21: case 22: case 30: case 31:          // breakable, coins
        case 39: case 40: case 45:                   // collision object, special, enter effect
            return {true, Category::Unclassified};
        default:  // 7 Decoration and anything unknown: no hitbox drawn
            return {false, Category::Unclassified};
    }
}

inline std::vector<Pt> circlePolygon(Pt center, float radius, std::size_t segments = 48) {
    std::vector<Pt> pts;
    if (!(radius > 0.f) || !std::isfinite(radius) || segments < 3) return pts;
    constexpr float kTwoPi = 6.2831853071795864769f;
    pts.reserve(segments);
    for (std::size_t i = 0; i < segments; ++i) {
        const float a = kTwoPi * static_cast<float>(i) / static_cast<float>(segments);
        pts.push_back({center.x + std::cos(a) * radius, center.y + std::sin(a) * radius});
    }
    return pts;
}

inline std::vector<Pt> rectPolygon(float minX, float minY, float maxX, float maxY) {
    return {{minX, minY}, {maxX, minY}, {maxX, maxY}, {minX, maxY}};
}

// A slope's triangle is its oriented box with one corner removed. The removed
// corner depends on the flip flags. INFERRED from the old prototype, not yet
// checked against the game (see the old docs/hitbox_mod.md, accuracy check).
inline int slopeExcludedCorner(bool flipX, bool flipY) {
    if (!flipX && !flipY) return 3;
    if (flipX && !flipY) return 2;
    if (!flipX && flipY) return 0;
    return 1;
}

inline std::vector<Pt> slopeTriangle(const std::array<Pt, 4>& corners, bool flipX, bool flipY) {
    const int skip = slopeExcludedCorner(flipX, flipY);
    std::vector<Pt> tri;
    tri.reserve(3);
    for (int i = 0; i < 4; ++i) {
        if (i != skip) tri.push_back(corners[static_cast<std::size_t>(i)]);
    }
    return tri;
}

// Index of the longest edge (point i to point i+1). Used for a hazardous slope's lethal edge.
inline std::size_t longestEdge(const std::vector<Pt>& poly) {
    std::size_t best = 0;
    float bestLen = -1.f;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const Pt& a = poly[i];
        const Pt& b = poly[(i + 1) % poly.size()];
        const float len = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
        if (len > bestLen) {
            bestLen = len;
            best = i;
        }
    }
    return best;
}

}  // namespace geodan::core::hitbox
