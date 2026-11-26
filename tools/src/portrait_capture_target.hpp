// Offscreen portrait render target for recorder frames.
// Renders one world subtree only; callers choose the world-space focus and scale.
#pragma once

#include <Geode/Geode.hpp>
#include <Geode/cocos/kazmath/include/kazmath/GL/matrix.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

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
        if (!beginAndDraw(worldRoot, focusWorld, worldToPointScale)) return false;
        m_target->end();
        return true;
    }

    // Renders like render(), and reads the target's pixels back before the pass
    // ends. The readback is exactly pixelWidth x pixelHeight RGBA8, rows
    // top-down, written to out (outSize must be pixelWidth * pixelHeight * 4).
    // This does not use CCRenderTexture::newCCImage, whose data length does not
    // match the target's pixel size when the content scale factor is above 1.
    bool renderAndRead(CCNode* worldRoot, CCPoint focusWorld, float worldToPointScale,
                       std::uint8_t* out, std::size_t outSize) {
        if (!out || !m_target || m_pixelWidth <= 0 || m_pixelHeight <= 0) return false;
        const std::size_t rowBytes = static_cast<std::size_t>(m_pixelWidth) * 4u;
        const std::size_t total = rowBytes * static_cast<std::size_t>(m_pixelHeight);
        if (outSize != total) return false;
        if (!beginAndDraw(worldRoot, focusWorld, worldToPointScale)) return false;

        if (m_scratch.size() != total) m_scratch.resize(total);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, m_pixelWidth, m_pixelHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                     m_scratch.data());
        m_target->end();

        // GL rows run bottom-up; the PNG encoder expects top-down.
        for (int y = 0; y < m_pixelHeight; ++y) {
            std::memcpy(out + static_cast<std::size_t>(y) * rowBytes,
                        m_scratch.data() +
                            static_cast<std::size_t>(m_pixelHeight - 1 - y) * rowBytes,
                        rowBytes);
        }
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
    // Starts the render pass and draws the subtree; the caller ends the pass.
    bool beginAndDraw(CCNode* worldRoot, CCPoint focusWorld, float worldToPointScale) {
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
        return true;
    }

    std::vector<std::uint8_t> m_scratch; // reused readback buffer, bottom-up rows
    CCRenderTexture* m_target = nullptr; // retained for this object's lifetime
    int m_pixelWidth = 0;
    int m_pixelHeight = 0;
    int m_pointWidth = 0;
    int m_pointHeight = 0;
};

} // namespace geodan::tools::capture
