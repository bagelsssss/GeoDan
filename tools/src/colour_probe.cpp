// Temporary probe: logs the border colours the game draws its debug hitboxes
// with, so the trigger colour can be confirmed before a recolouring rule is
// written. Each distinct (colour, bounding-box size) is logged once.
// Our own overlay draws also pass through here; ignore colours that match the
// overlay palette. Remove this file once the trigger rule is in.
#include <Geode/Geode.hpp>
#include <Geode/modify/CCDrawNode.hpp>

#include <cmath>
#include <set>
#include <tuple>

using namespace geode::prelude;

namespace {
constexpr std::size_t kMaxLogged = 500;
std::set<std::tuple<int, int, int, int, int, int>> g_seen;
}

class $modify(GeoDanColourProbe, CCDrawNode) {
    void drawPolygon(CCPoint* verts, unsigned int count, ccColor4F fillColor,
                     float borderWidth, ccColor4F borderColor) {
        if (verts && count > 0 && g_seen.size() < kMaxLogged) {
            float minX = verts[0].x, maxX = verts[0].x;
            float minY = verts[0].y, maxY = verts[0].y;
            for (unsigned int i = 1; i < count; ++i) {
                minX = std::min(minX, verts[i].x);
                maxX = std::max(maxX, verts[i].x);
                minY = std::min(minY, verts[i].y);
                maxY = std::max(maxY, verts[i].y);
            }
            const int r = static_cast<int>(std::lround(borderColor.r * 255.f));
            const int g = static_cast<int>(std::lround(borderColor.g * 255.f));
            const int b = static_cast<int>(std::lround(borderColor.b * 255.f));
            const int a = static_cast<int>(std::lround(borderColor.a * 255.f));
            const int w = static_cast<int>(std::lround(maxX - minX));
            const int h = static_cast<int>(std::lround(maxY - minY));
            const auto key = std::make_tuple(r, g, b, a, w, h);
            if (g_seen.insert(key).second) {
                log::info("colour_probe border=({},{},{},{}) bbox={}x{} verts={} border_width={:.2f}",
                          r, g, b, a, w, h, count, borderWidth);
            }
        }
        CCDrawNode::drawPolygon(verts, count, fillColor, borderWidth, borderColor);
    }
};
