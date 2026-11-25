// Shared diagnostic overlay drawing for live gameplay and recorder targets.
// This file contains rendering only; it does not own scene nodes or game state.
#pragma once

#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>
#include <array>
#include <vector>

#include "core/hitbox_model.hpp"
#include "core/hitbox_trail.hpp"

namespace geodan::tools::overlay {
namespace hb = geodan::core::hitbox;
namespace tr = geodan::core::trail;

// Colours are a screenshot-parser contract (see the old docs/hitbox_mod.md).
const ccColor4F kHazard = ccc4f(1.00f, 0.18f, 0.18f, 1.f);
const ccColor4F kSolid = ccc4f(0.20f, 0.50f, 1.00f, 1.f);
const ccColor4F kModifier = ccc4f(0.18f, 0.88f, 0.44f, 1.f);
const ccColor4F kUnclassified = ccc4f(1.00f, 0.20f, 0.90f, 1.f);
const ccColor4F kNoFill = ccc4f(0.f, 0.f, 0.f, 0.f);
const float kTrailGrey = 0.33f;     // neutral grey, never red or blue
const float kTrailOpacity = 0.6f;
const float kLineWidth = 0.65f;
const float kHazardLineWidth = 1.0f;
const float kViewMargin = 120.f;    // extra world units around the screen

inline const ccColor4F& colourOf(hb::Category c) {
    switch (c) {
        case hb::Category::Hazard: return kHazard;
        case hb::Category::Solid: return kSolid;
        case hb::Category::Modifier: return kModifier;
        case hb::Category::Unclassified: return kUnclassified;
    }
    return kUnclassified;
}

// Cocos CCDrawNode blends premultiplied colour, so the colour must be scaled by alpha first.
inline ccColor4F premultiplied(float grey, float alpha) {
    return ccc4f(grey * alpha, grey * alpha, grey * alpha, alpha);
}

inline CCRect visibleRect(CCNode* layer, float margin) {
    const auto win = CCDirector::sharedDirector()->getWinSize();
    const CCPoint corners[4] = {
        ccp(0.f, 0.f), ccp(win.width, 0.f), ccp(win.width, win.height), ccp(0.f, win.height)};
    float minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f;
    for (const auto& c : corners) {
        const auto p = layer->convertToNodeSpace(c);
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    return CCRectMake(minX - margin, minY - margin,
                      (maxX - minX) + margin * 2.f, (maxY - minY) + margin * 2.f);
}

// Builds the outline for one object. Returns false when nothing should be drawn.
inline bool buildShape(GameObject* o, bool showUnclassified, std::vector<hb::Pt>& points,
                hb::Category& category, bool& hazardEdge, hb::Pt& edgeA, hb::Pt& edgeB) {
    if (!o || o->getGroupDisabled() || o->m_isNoTouch) return false;

    const auto cls = hb::classifyType(static_cast<int>(o->getType()));
    if (!cls.draw) return false;
    if (cls.category == hb::Category::Unclassified && !showUnclassified) return false;
    category = cls.category;

    // Circular objects (orbs, saws) are drawn as circles from their radius.
    const float scale = std::max(std::abs(o->m_scaleX), std::abs(o->m_scaleY));
    const float radius = o->m_objectRadius * scale;
    if (radius > 0.f) {
        const auto p = o->getPosition();
        points = hb::circlePolygon({p.x, p.y}, radius);
        return !points.empty();
    }

    // The game's rotated collision box when present, else the axis-aligned rect.
    std::array<CCPoint, 4> corners{};
    bool haveCorners = false;
    if (o->m_orientedBox) {
        if (auto* box = o->getOrientedBox()) {
            for (std::size_t i = 0; i < 4; ++i) corners[i] = box->m_corners[i];
            haveCorners = true;
        }
    }
    if (!haveCorners) {
        const auto r = o->getObjectRect();
        corners = {ccp(r.getMinX(), r.getMinY()), ccp(r.getMaxX(), r.getMinY()),
                   ccp(r.getMaxX(), r.getMaxY()), ccp(r.getMinX(), r.getMaxY())};
    }
    std::array<hb::Pt, 4> pts{};
    for (std::size_t i = 0; i < 4; ++i) pts[i] = {corners[i].x, corners[i].y};

    // Slopes are a triangle; a hazardous slope also has a lethal longest edge.
    if (o->getType() == GameObjectType::Slope) {
        points = hb::slopeTriangle(pts, o->isFlipX(), o->isFlipY());
        if (o->m_slopeIsHazard && points.size() == 3) {
            const auto e = hb::longestEdge(points);
            hazardEdge = true;
            edgeA = points[e];
            edgeB = points[(e + 1) % 3];
        }
    } else {
        points.assign(pts.begin(), pts.end());
    }
    return points.size() >= 3;
}

inline void drawOutline(CCDrawNode* node, const std::vector<hb::Pt>& pts, hb::Category category,
                 bool hazardEdge, hb::Pt edgeA, hb::Pt edgeB) {
    std::vector<CCPoint> cc;
    cc.reserve(pts.size());
    for (const auto& p : pts) cc.push_back(ccp(p.x, p.y));
    const float width = category == hb::Category::Hazard ? kHazardLineWidth : kLineWidth;
    node->drawPolygon(cc.data(), static_cast<unsigned int>(cc.size()), kNoFill, width,
                      colourOf(category));
    if (hazardEdge) {
        node->drawSegment(ccp(edgeA.x, edgeA.y), ccp(edgeB.x, edgeB.y), kHazardLineWidth, kHazard);
    }
}

// Draws one player's trail: older samples fainter, neutral grey only.
inline void drawTrail(CCDrawNode* node, const tr::Trail& trail) {
    if (trail.empty()) return;
    constexpr std::size_t kMaxDrawn = 240;
    const std::size_t stride =
        std::max<std::size_t>(1, (trail.size() + kMaxDrawn - 1) / kMaxDrawn);

    const auto drawSample = [&](std::size_t index) {
        const auto& sample = trail[index];
        const float progress = trail.size() <= 1
            ? 1.f
            : static_cast<float>(index) / static_cast<float>(trail.size() - 1);
        const float alpha = std::clamp(kTrailOpacity * (0.15f + 0.85f * progress), 0.f, 1.f);

        const auto& b = sample.outer;
        CCPoint outer[4] = {ccp(b.left, b.bottom), ccp(b.right, b.bottom),
                            ccp(b.right, b.top), ccp(b.left, b.top)};
        node->drawPolygon(outer, 4, premultiplied(kTrailGrey, alpha * 0.08f), 0.9f,
                          premultiplied(kTrailGrey, alpha));

        if (sample.collisionBlock) {
            const auto& c = *sample.collisionBlock;
            CCPoint inner[4] = {ccp(c.left, c.bottom), ccp(c.right, c.bottom),
                                ccp(c.right, c.top), ccp(c.left, c.top)};
            node->drawPolygon(inner, 4, kNoFill, 0.7f,
                              premultiplied(kTrailGrey, alpha * 0.85f));
        }
    };

    for (std::size_t i = 0; i < trail.size(); i += stride) drawSample(i);
    if ((trail.size() - 1) % stride != 0) drawSample(trail.size() - 1);
}



inline void drawHitboxes(CCDrawNode* node, CCArray* objects, CCNode* objectLayer,
                         bool enabled, bool showUnclassified) {
    if (!node || !objectLayer) return;
    node->clear();
    node->setVisible(enabled);
    if (!enabled || !objects) return;

    const auto view = visibleRect(objectLayer, kViewMargin);
    for (unsigned int i = 0; i < objects->count(); ++i) {
        auto* object = static_cast<GameObject*>(objects->objectAtIndex(i));
        if (!object || !view.containsPoint(object->getPosition())) continue;

        std::vector<hb::Pt> points;
        hb::Category category = hb::Category::Unclassified;
        bool hazardEdge = false;
        hb::Pt edgeA, edgeB;
        if (!buildShape(object, showUnclassified, points, category,
                        hazardEdge, edgeA, edgeB))
            continue;
        drawOutline(node, points, category, hazardEdge, edgeA, edgeB);
    }
}

} // namespace geodan::tools::overlay
