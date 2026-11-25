// Portable geometry contract shared by the Recorder writer and Image Parser.
// Coordinates stored here are capture-pixel coordinates, origin at top-left.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <set>
#include <string_view>
#include <vector>

namespace geodan::core::recorder {

enum class CapturePhase { StartIdle, ActiveGameplay, Paused, Respawn, Transition, Unknown };
enum class GeometryStatus { Valid, Unsupported };
enum class GeometryEntityKind { Player, Hazard };

struct CapturePoint {
    double x = 0.0;
    double y = 0.0;
};

struct CaptureBounds {
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;

    bool valid() const {
        return std::isfinite(left) && std::isfinite(top) &&
               std::isfinite(right) && std::isfinite(bottom) &&
               left < right && top < bottom;
    }
};

struct GeometryEntity {
    GeometryEntityKind kind = GeometryEntityKind::Hazard;
    std::uint64_t id = 0;
    CaptureBounds bounds;
};

struct FrameGeometry {
    std::uint64_t sequence = 0;
    CapturePhase phase = CapturePhase::Unknown;
    GeometryStatus status = GeometryStatus::Unsupported;
    std::vector<GeometryEntity> entities;

    bool valid() const {
        std::set<std::pair<GeometryEntityKind, std::uint64_t>> ids;
        for (const auto& entity : entities) {
            if (!entity.bounds.valid() || !ids.emplace(entity.kind, entity.id).second)
                return false;
        }
        return true;
    }
};

struct CaptureTransform {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    double contentScale = 0.0;
    double worldToPointScale = 0.0;
    CapturePoint focusWorld;

    bool valid() const {
        return width > 0 && height > 0 &&
               std::isfinite(contentScale) && contentScale > 0.0 &&
               std::isfinite(worldToPointScale) && worldToPointScale > 0.0 &&
               std::isfinite(focusWorld.x) && std::isfinite(focusWorld.y);
    }

    // Mirrors the portrait render transform: world focus at image center,
    // Cocos y-up converted to PNG/capture y-down.
    CapturePoint worldToCapture(CapturePoint world) const {
        const double scale = contentScale * worldToPointScale;
        return {
            static_cast<double>(width) * 0.5 + (world.x - focusWorld.x) * scale,
            static_cast<double>(height) * 0.5 - (world.y - focusWorld.y) * scale
        };
    }
};

inline constexpr std::string_view phaseName(CapturePhase phase) {
    switch (phase) {
        case CapturePhase::StartIdle: return "start_idle";
        case CapturePhase::ActiveGameplay: return "active_gameplay";
        case CapturePhase::Paused: return "paused";
        case CapturePhase::Respawn: return "respawn";
        case CapturePhase::Transition: return "transition";
        case CapturePhase::Unknown: return "unknown";
    }
    return "unknown";
}

inline constexpr std::string_view geometryStatusName(GeometryStatus status) {
    return status == GeometryStatus::Valid ? "valid" : "unsupported";
}

inline constexpr std::string_view geometryEntityName(GeometryEntityKind kind) {
    return kind == GeometryEntityKind::Player ? "player" : "hazard";
}

inline CaptureBounds boundsFromWorldPoints(const std::vector<CapturePoint>& points,
                                           const CaptureTransform& transform) {
    if (points.size() < 2 || !transform.valid()) return {};
    CaptureBounds result{
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()
    };
    for (const auto& point : points) {
        if (!std::isfinite(point.x) || !std::isfinite(point.y)) return {};
        const auto mapped = transform.worldToCapture(point);
        result.left = std::min(result.left, mapped.x);
        result.top = std::min(result.top, mapped.y);
        result.right = std::max(result.right, mapped.x);
        result.bottom = std::max(result.bottom, mapped.y);
    }
    return result;
}

inline CaptureBounds clipToCapture(CaptureBounds bounds, std::uint32_t width,
                                   std::uint32_t height) {
    if (!bounds.valid() || width == 0 || height == 0) return {};
    bounds.left = std::clamp(bounds.left, 0.0, static_cast<double>(width));
    bounds.right = std::clamp(bounds.right, 0.0, static_cast<double>(width));
    bounds.top = std::clamp(bounds.top, 0.0, static_cast<double>(height));
    bounds.bottom = std::clamp(bounds.bottom, 0.0, static_cast<double>(height));
    return bounds.valid() ? bounds : CaptureBounds{};
}

inline bool intersectsCapture(const CaptureBounds& bounds, std::uint32_t width,
                              std::uint32_t height) {
    return bounds.valid() && bounds.right > 0.0 && bounds.bottom > 0.0 &&
           bounds.left < static_cast<double>(width) &&
           bounds.top < static_cast<double>(height);
}

// One sentinel row per frame makes even empty/unsupported frames explicit.
// Entity rows carry axis-aligned envelopes of the source collision geometry.
// For rotated/circular shapes these envelopes are conservative, not exact.
inline bool writeGeometryCsvFrame(std::ostream& output, const FrameGeometry& frame) {
    if (!frame.valid()) return false;
    const auto phase = phaseName(frame.phase);
    const auto status = geometryStatusName(frame.status);
    output << std::setprecision(12);
    output << frame.sequence << ',' << phase << ',' << status
           << ",frame,,,,,\n";
    for (const auto& entity : frame.entities) {
        output << frame.sequence << ',' << phase << ',' << status << ','
               << geometryEntityName(entity.kind) << ',' << entity.id << ','
               << entity.bounds.left << ',' << entity.bounds.top << ','
               << entity.bounds.right << ',' << entity.bounds.bottom << '\n';
    }
    return static_cast<bool>(output);
}

} // namespace geodan::core::recorder
