// GeoDan Tools - gameplay overlay. Draws object hitboxes and the player's
// hitbox trail on top of the level. Diagnostic only: it reads game state and
// never changes collision, physics or level objects.
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>
#include <array>
#include <optional>
#include <vector>

#include "core/hitbox_model.hpp"
#include "core/hitbox_trail.hpp"
#include "trajectory.hpp"

using namespace geode::prelude;
namespace hb = geodan::core::hitbox;
namespace tr = geodan::core::trail;

namespace {

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

const ccColor4F& colourOf(hb::Category c) {
    switch (c) {
        case hb::Category::Hazard: return kHazard;
        case hb::Category::Solid: return kSolid;
        case hb::Category::Modifier: return kModifier;
        case hb::Category::Unclassified: return kUnclassified;
    }
    return kUnclassified;
}

// Cocos CCDrawNode blends premultiplied colour, so the colour must be scaled by alpha first.
ccColor4F premultiplied(float grey, float alpha) {
    return ccc4f(grey * alpha, grey * alpha, grey * alpha, alpha);
}

CCRect visibleRect(CCNode* layer, float margin) {
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

std::vector<hb::Pt> toPts(const std::array<CCPoint, 4>& c) {
    std::vector<hb::Pt> out;
    out.reserve(4);
    for (const auto& p : c) out.push_back({p.x, p.y});
    return out;
}

// Builds the outline for one object. Returns false when nothing should be drawn.
bool buildShape(GameObject* o, bool showUnclassified, std::vector<hb::Pt>& points,
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

void drawOutline(CCDrawNode* node, const std::vector<hb::Pt>& pts, hb::Category category,
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
void drawTrail(CCDrawNode* node, const tr::Trail& trail) {
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

std::optional<tr::Snapshot> snapshotOf(PlayerObject* p) {
    if (!p || !p->isVisible()) return std::nullopt;
    const auto outer = p->getObjectRect();
    const auto block = p->getObjectRect(0.3f, 0.3f);
    tr::Snapshot s;
    s.outer = {outer.getMinX(), outer.getMinY(), outer.getMaxX(), outer.getMaxY()};
    s.collisionBlock = tr::Bounds{block.getMinX(), block.getMinY(), block.getMaxX(), block.getMaxY()};
    return s;
}

}  // namespace

class $modify(GeoDanToolsPlayLayer, PlayLayer) {
    struct Fields {
        CCDrawNode* overlay = nullptr;
        CCDrawNode* trailNode = nullptr;
        tr::Tracker tracker;
    };

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& f = *m_fields.self();
        auto* mod = Mod::get();

        // Trail: one sample per tick, recorded even while the overlay is hidden
        // so switching it on shows history straight away.
        f.tracker.observeTick(dt, snapshotOf(m_player1), snapshotOf(m_player2));
        updateGeoDanTrajectory(this, dt);

        const bool showTrail = mod->getSettingValue<bool>("hitbox-trail");
        const bool showHitboxes = mod->getSettingValue<bool>("hitboxes");
        const bool showUnclassified = mod->getSettingValue<bool>("unclassified");

        if (!m_objectLayer) return;

        if (!f.overlay) {
            f.overlay = CCDrawNode::create();
            if (!f.overlay) return;
            m_objectLayer->addChild(f.overlay, 9999);
        }
        if (!f.trailNode) {
            f.trailNode = CCDrawNode::create();
            if (!f.trailNode) return;
            m_objectLayer->addChild(f.trailNode, 10000);
        }

        f.overlay->clear();
        f.overlay->setVisible(showHitboxes);
        if (showHitboxes && m_objects) {
            const auto view = visibleRect(m_objectLayer, kViewMargin);
            for (unsigned int i = 0; i < m_objects->count(); ++i) {
                auto* o = static_cast<GameObject*>(m_objects->objectAtIndex(i));
                if (!o || !view.containsPoint(o->getPosition())) continue;

                std::vector<hb::Pt> points;
                hb::Category category = hb::Category::Unclassified;
                bool hazardEdge = false;
                hb::Pt edgeA, edgeB;
                if (!buildShape(o, showUnclassified, points, category,
                                hazardEdge, edgeA, edgeB))
                    continue;
                drawOutline(f.overlay, points, category, hazardEdge, edgeA, edgeB);
            }
        }

        f.trailNode->clear();
        f.trailNode->setVisible(showTrail);
        if (showTrail) {
            drawTrail(f.trailNode, f.tracker.trail(0));
            drawTrail(f.trailNode, f.tracker.trail(1));
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->tracker.reset();
        resetGeoDanTrajectory(this);
    }

    void onQuit() {
        PlayLayer::onQuit();
        cleanupGeoDanTrajectory(this);
    }
};
