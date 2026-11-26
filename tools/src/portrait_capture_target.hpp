// Offscreen portrait render target for recorder frames.
// Renders one world subtree only; callers choose the world-space focus and scale.
#pragma once

#include <Geode/Geode.hpp>
#include <Geode/cocos/kazmath/include/kazmath/GL/matrix.h>

#include <cmath>
#include <optional>

namespace geodan::tools::capture {

class PortraitCaptureTarget final {
public:
    PortraitCaptureTarget() = default;
    PortraitCaptureTarget(const PortraitCaptureTarget&) = delete;
    PortraitCaptureTarget& operator=(const PortraitCaptureTarget&) = delete;

    ~PortraitCaptureTarget() {
        if (m_target) m_target->release();
    }

    // Requested dimensions are output pixels, not Cocos points. Cocos render
    // textures are created in points, so account for the active content scale
    // factor and reject dimensions that cannot be represented exactly.
    bool initialize(int pixelWidth = 360, int pixelHeight = 1080) {
        if (m_target || pixelWidth <= 0 || pixelHeight <= 0) return false;

        const float contentScale = CC_CONTENT_SCALE_FACTOR();
        if (!std::isfinite(contentScale) || contentScale <= 0.f) return false;

        const int pointWidth = static_cast<int>(std::lround(pixelWidth / contentScale));
        const int pointHeight = static_cast<int>(std::lround(pixelHeight / contentScale));
        if (pointWidth <= 0 || pointHeight <= 0 ||
            std::lround(pointWidth * contentScale) != pixelWidth ||
            std::lround(pointHeight * contentScale) != pixelHeight)
            return false;

        auto* candidate = CCRenderTexture::create(
            pointWidth, pointHeight, kCCTexture2DPixelFormat_RGBA8888);
        if (!candidate || !candidate->getSprite() || !candidate->getSprite()->getTexture())
            return false;

        const auto actualPixels = candidate->getSprite()->getTexture()->getContentSizeInPixels();
        if (static_cast<int>(std::lround(actualPixels.width)) != pixelWidth ||
            static_cast<int>(std::lround(actualPixels.height)) != pixelHeight)
            return false;

        candidate->retain();
        m_target = candidate;
        m_pixelWidth = pixelWidth;
        m_pixelHeight = pixelHeight;
        m_pointWidth = pointWidth;
        m_pointHeight = pointHeight;
        return true;
    }

    // focusWorld is expressed in Cocos world coordinates. worldToPointScale
    // is explicit so the caller must choose and validate the portrait crop;
    // this class does not silently zoom or alter the live game camera.
    bool render(CCNode* worldRoot, CCPoint focusWorld, float worldToPointScale) {
        if (!m_target || !worldRoot || !std::isfinite(worldToPointScale) ||
            worldToPointScale <= 0.f)
            return false;

        m_target->beginWithClear(0.f, 0.f, 0.f, 1.f);

        // Map the requested world-space focus to the center of the portrait
        // target, then apply the live ancestor transforms before visiting the
        // supplied subtree. This leaves the normal scene graph untouched.
        kmGLPushMatrix();
        kmGLTranslatef(m_pointWidth * 0.5f, m_pointHeight * 0.5f, 0.f);
        kmGLScalef(worldToPointScale, worldToPointScale, 1.f);
        kmGLTranslatef(-focusWorld.x, -focusWorld.y, 0.f);
        worldRoot->transformAncestors();
        worldRoot->visit();
        kmGLPopMatrix();

        m_target->end();
        return true;
    }

    CCRenderTexture* renderTexture() const { return m_target; }
    int pixelWidth() const { return m_pixelWidth; }
    int pixelHeight() const { return m_pixelHeight; }
    int pointWidth() const { return m_pointWidth; }
    int pointHeight() const { return m_pointHeight; }

    static std::optional<CCPoint> worldPositionOf(CCNode* node) {
        if (!node) return std::nullopt;
        auto* parent = node->getParent();
        return parent ? std::optional<CCPoint>(parent->convertToWorldSpace(node->getPosition()))
                      : std::optional<CCPoint>(node->getPosition());
    }

private:
    CCRenderTexture* m_target = nullptr; // retained for this object's lifetime
    int m_pixelWidth = 0;
    int m_pixelHeight = 0;
    int m_pointWidth = 0;
    int m_pointHeight = 0;
};

} // namespace geodan::tools::capture
