// Game-facing adapter: snapshots the existing diagnostic collision geometry
// into portable capture-pixel bounds. This is metadata only; it never changes
// gameplay collision or rendering.
#pragma once

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "core/hitbox_model.hpp"
#include "core/recorder_geometry.hpp"
#include "overlay_renderer.hpp"

namespace geodan::tools::capture {

namespace hb = geodan::core::hitbox;
namespace rec = geodan::core::recorder;
namespace overlay = geodan::tools::overlay;

inline rec::CaptureBounds mapObjectLayerRect(
    const CCRect& rect, CCNode* objectLayer, const rec::CaptureTransform& transform) {
    if (!objectLayer) return {};
    const CCPoint corners[] = {
        ccp(rect.getMinX(), rect.getMinY()),
        ccp(rect.getMaxX(), rect.getMinY()),
        ccp(rect.getMaxX(), rect.getMaxY()),
        ccp(rect.getMinX(), rect.getMaxY())
    };
    std::vector<rec::CapturePoint> world;
    world.reserve(4);
    for (const auto& corner : corners) {
        const auto point = objectLayer->convertToWorldSpace(corner);
        world.push_back({point.x, point.y});
    }
    return rec::boundsFromWorldPoints(world, transform);
}

inline rec::CaptureBounds mapObjectLayerPolygon(
    const std::vector<hb::Pt>& polygon, CCNode* objectLayer,
    const rec::CaptureTransform& transform) {
    if (!objectLayer || polygon.size() < 3) return {};
    std::vector<rec::CapturePoint> world;
    world.reserve(polygon.size());
    for (const auto& vertex : polygon) {
        const auto point = objectLayer->convertToWorldSpace(ccp(vertex.x, vertex.y));
        world.push_back({point.x, point.y});
    }
    return rec::boundsFromWorldPoints(world, transform);
}

inline void markUnsupported(rec::FrameGeometry& frame, const char* reason, int objectType = -1) {
    frame.status = rec::GeometryStatus::Unsupported;
    frame.unsupportedNotes.push_back({reason, objectType});
}

inline void addPlayer(rec::FrameGeometry& frame, PlayerObject* player,
                      std::uint64_t playerId, CCNode* objectLayer,
                      const rec::CaptureTransform& transform) {
    if (!player || !player->isVisible()) {
        markUnsupported(frame, "player_hidden");
        return;
    }
    // This is the inner collision block already used by the live hitbox trail.
    // It is a diagnostic proxy until checked against the engine collision path.
    const auto bounds = mapObjectLayerRect(
        player->getObjectRect(0.3f, 0.3f), objectLayer, transform);
    if (!bounds.valid()) {
        markUnsupported(frame, "player_bounds_invalid");
        return;
    }
    const auto clipped = rec::clipToCapture(bounds, transform.width, transform.height);
    if (clipped.valid())
        frame.entities.push_back({rec::GeometryEntityKind::Player, playerId, clipped});
}

inline rec::FrameGeometry snapshotGeometry(
    PlayLayer* layer, std::uint64_t sequence, rec::CapturePhase phase,
    const rec::CaptureTransform& transform) {
    rec::FrameGeometry frame;
    frame.sequence = sequence;
    frame.phase = phase;
    frame.status = rec::GeometryStatus::Valid;
    if (phase == rec::CapturePhase::Unknown) markUnsupported(frame, "phase_unknown");

    if (!layer || !layer->m_objectLayer || !layer->m_objects || !transform.valid()) {
        markUnsupported(frame, "layer_missing");
        return frame;
    }

    addPlayer(frame, layer->m_player1, 1, layer->m_objectLayer, transform);
    if (layer->m_player2 && layer->m_player2->isVisible())
        addPlayer(frame, layer->m_player2, 2, layer->m_objectLayer, transform);

    for (unsigned int i = 0; i < layer->m_objects->count(); ++i) {
        auto* object = static_cast<GameObject*>(layer->m_objects->objectAtIndex(i));
        if (!object || object->getGroupDisabled() || object->m_isNoTouch) continue;

        const int type = static_cast<int>(object->getType());
        if (type == 7) continue; // Decoration is explicitly non-collidable.

        const auto classification = hb::classifyType(type);
        const bool hazardousSlope =
            object->getType() == GameObjectType::Slope && object->m_slopeIsHazard;

        if (classification.category == hb::Category::Unclassified || !classification.draw) {
            // Unknown interactive types are not silently interpreted as safe.
            markUnsupported(frame, "unclassified", type);
            continue;
        }
        if (classification.category != hb::Category::Hazard && !hazardousSlope)
            continue;

        std::vector<hb::Pt> polygon;
        hb::Category category = hb::Category::Unclassified;
        bool hazardEdge = false;
        hb::Pt edgeA, edgeB;
        if (!overlay::buildShape(object, true, polygon, category,
                                  hazardEdge, edgeA, edgeB)) {
            markUnsupported(frame, "shape_failed", type);
            continue;
        }
        // A lethal slope edge is a segment, not a filled polygon. The current
        // parser contract stores rectangles, so preserve uncertainty explicitly.
        if (hazardEdge || polygon.size() < 3) {
            markUnsupported(frame, hazardEdge ? "hazard_edge" : "polygon_too_small", type);
            continue;
        }

        const auto bounds = mapObjectLayerPolygon(polygon, layer->m_objectLayer, transform);
        if (!bounds.valid()) {
            markUnsupported(frame, "hazard_bounds_invalid", type);
            continue;
        }
        if (!rec::intersectsCapture(bounds, transform.width, transform.height)) continue;
        const auto clipped = rec::clipToCapture(bounds, transform.width, transform.height);
        if (!clipped.valid()) {
            markUnsupported(frame, "hazard_clip_invalid", type);
            continue;
        }
        // Object-array index is stable for the duration of a level session and
        // unique within each frame. It is not a persistent editor object ID.
        frame.entities.push_back({rec::GeometryEntityKind::Hazard,
                                  static_cast<std::uint64_t>(i) + 1, clipped});
    }

    if (std::none_of(frame.entities.begin(), frame.entities.end(), [](const auto& entity) {
            return entity.kind == rec::GeometryEntityKind::Player;
        })) {
        markUnsupported(frame, "no_player");
    }
    return frame;
}

} // namespace geodan::tools::capture
