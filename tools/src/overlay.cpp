// GeoDan Tools - gameplay overlay. Draws object hitboxes and the player's
// hitbox trail on top of the level, plus a hitbox-only portrait preview.
// Diagnostic only: it reads game state and never changes collision, physics or
// level objects.
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>
#include <cmath>
#include <array>
#include <optional>
#include <vector>

#include "core/hitbox_model.hpp"
#include "core/hitbox_trail.hpp"
#include "overlay_renderer.hpp"

using namespace geode::prelude;
namespace hb = geodan::core::hitbox;
namespace tr = geodan::core::trail;
namespace overlay = geodan::tools::overlay;

namespace {

// The preview is refreshed this often; the last image stays up in between.
constexpr float kPreviewInterval = 0.1f;
// Portrait crop is one third as wide as the window is tall (360x1080 shape).
constexpr float kPortraitRatio = 1.f / 3.f;
// Player rectangles use the amber of the deprecated hitbox tool.
const ccColor4F kPlayerAmber = ccc4f(1.f, 0.78f, 0.15f, 1.f);

std::optional<tr::Snapshot> snapshotOf(PlayerObject* p) {
    if (!p || !p->isVisible()) return std::nullopt;
    const auto outer = p->getObjectRect();
    const auto block = p->getObjectRect(0.3f, 0.3f);
    tr::Snapshot s;
    s.outer = {outer.getMinX(), outer.getMinY(), outer.getMaxX(), outer.getMaxY()};
    s.collisionBlock = tr::Bounds{block.getMinX(), block.getMinY(), block.getMaxX(), block.getMaxY()};
    return s;
}

// Screen position of a node: its parent's position converted to world space.
CCPoint screenPositionOf(CCNode* node) {
    auto* parent = node->getParent();
    return parent ? parent->convertToWorldSpace(node->getPosition()) : node->getPosition();
}

}  // namespace

class $modify(GeoDanToolsPlayLayer, PlayLayer) {
    struct Fields {
        CCDrawNode* overlay = nullptr;
        CCDrawNode* trailNode = nullptr;
        tr::Tracker tracker;
        // Hitbox-only preview. previewRoot is a hidden child of the object layer;
        // it is visited once per refresh into a window-sized target, and the
        // portrait crop around the player is shown as a sprite (GPU only).
        CCNode* previewRoot = nullptr;
        CCDrawNode* previewDraw = nullptr;
        CCRenderTexture* previewTarget = nullptr;
        CCSprite* previewSprite = nullptr;
        float previewAccum = 0.f;
        int previewFrames = 0;
    };

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& f = *m_fields.self();
        auto* mod = Mod::get();

        // Trail: one sample per tick, recorded even while the overlay is hidden
        // so switching it on shows history straight away.
        f.tracker.observeTick(dt, snapshotOf(m_player1), snapshotOf(m_player2));

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

        overlay::drawHitboxes(f.overlay, m_objects, m_objectLayer,
                              showHitboxes, showUnclassified);

        f.trailNode->clear();
        f.trailNode->setVisible(showTrail);
        if (showTrail) {
            overlay::drawTrail(f.trailNode, f.tracker.trail(0));
            overlay::drawTrail(f.trailNode, f.tracker.trail(1));
        }

        const bool portraitPreview = mod->getSettingValue<bool>("portrait-preview-test");
        if (!portraitPreview) {
            if (f.previewSprite) f.previewSprite->setVisible(false);
            return;
        }
        f.previewAccum += dt;
        if (f.previewAccum < kPreviewInterval) return;
        f.previewAccum = 0.f;
        updateHitboxPreview();
    }

    // Rebuilds the preview from the scene's own transforms: no custom GL matrices
    // and no readback to the CPU.
    void updateHitboxPreview() {
        auto& f = *m_fields.self();
        const auto win = CCDirector::sharedDirector()->getWinSize();
        if (!m_objectLayer || win.width <= 0.f || win.height <= 0.f) {
            return;
        }

        if (!f.previewTarget) {
            f.previewTarget = CCRenderTexture::create(
                static_cast<int>(win.width), static_cast<int>(win.height),
                kCCTexture2DPixelFormat_RGBA8888);
            f.previewRoot = CCNode::create();
            f.previewDraw = CCDrawNode::create();
            f.previewSprite = CCSprite::create();
            if (!f.previewTarget || !f.previewRoot || !f.previewDraw || !f.previewSprite) {
                return;
            }
            f.previewRoot->addChild(f.previewDraw);
            f.previewRoot->setVisible(false);
            m_objectLayer->addChild(f.previewRoot, 10002);
            this->addChild(f.previewSprite, 100000);
        }

        // Focus: player 1, or the midpoint of both players in dual mode. The view
        // widens when they are far apart vertically so both stay visible.
        CCPoint focusWorld = screenPositionOf(m_player1);
        float span = 0.f;
        if (m_gameState.m_isDualMode && m_player2) {
            const CCPoint second = screenPositionOf(m_player2);
            focusWorld = ccp((focusWorld.x + second.x) * 0.5f, (focusWorld.y + second.y) * 0.5f);
            span = std::abs(screenPositionOf(m_player1).y - second.y);
        }
        const float viewHeight = std::max(win.height * 2.f, span * 1.5f + 200.f);
        const float zoom = win.height / viewHeight;
        const CCPoint focusLocal = m_objectLayer->convertToNodeSpace(focusWorld);

        // Zoom about the focus: its own transform keeps focusLocal fixed.
        f.previewRoot->setAnchorPoint(ccp(0.f, 0.f));
        f.previewRoot->setScale(zoom);
        f.previewRoot->setPosition(ccp(focusLocal.x * (1.f - zoom), focusLocal.y * (1.f - zoom)));

        // Hitboxes only: objects and players inside the preview's own view.
        f.previewDraw->clear();
        const float halfW = win.width * 0.5f / zoom + 120.f;
        const float halfH = win.height * 0.5f / zoom + 120.f;
        const CCRect cull = CCRectMake(focusLocal.x - halfW, focusLocal.y - halfH,
                                       halfW * 2.f, halfH * 2.f);
        overlay::drawHitboxes(f.previewDraw, m_objects, m_objectLayer, true, true, &cull);
        for (PlayerObject* player : {m_player1, m_player2}) {
            if (!player || !player->isVisible() || (player == m_player2 && !m_gameState.m_isDualMode))
                continue;
            const auto rect = player->getObjectRect();
            const CCPoint a = ccp(rect.getMinX(), rect.getMinY());
            const CCPoint b = ccp(rect.getMinX(), rect.getMaxY());
            const CCPoint c = ccp(rect.getMaxX(), rect.getMaxY());
            const CCPoint d = ccp(rect.getMaxX(), rect.getMinY());
            f.previewDraw->drawSegment(a, b, 1.5f, kPlayerAmber);
            f.previewDraw->drawSegment(b, c, 1.5f, kPlayerAmber);
            f.previewDraw->drawSegment(c, d, 1.5f, kPlayerAmber);
            f.previewDraw->drawSegment(d, a, 1.5f, kPlayerAmber);
        }

        // Render the hidden subtree with the scene's ancestor transforms applied.
        f.previewRoot->setVisible(true);
        f.previewTarget->beginWithClear(0.f, 0.f, 0.f, 1.f);
        f.previewRoot->transformAncestors();
        f.previewRoot->visit();
        f.previewTarget->end();
        f.previewRoot->setVisible(false);

        // Portrait crop centred on the focus, shown as a flipped texture region.
        auto* texture = f.previewTarget->getSprite()->getTexture();
        if (!texture) {
            return;
        }
        const float cropW = win.height * kPortraitRatio;
        const float left = std::clamp(focusWorld.x - cropW * 0.5f, 0.f, win.width - cropW);
        const float pixels = static_cast<float>(texture->getPixelsWide()) / win.width;
        f.previewSprite->setTexture(texture);
        f.previewSprite->setTextureRect(CCRectMake(left * pixels, 0.f,
                                                   cropW * pixels, win.height * pixels));
        f.previewSprite->setFlipY(true);
        f.previewSprite->setContentSize(CCSizeMake(cropW, win.height));

        // Display at up to 42% of the window width and 78% of its height.
        const float displayH = std::min(win.height * 0.78f, win.width * 0.42f / kPortraitRatio);
        const float k = displayH / win.height;
        f.previewSprite->setScale(k);
        f.previewSprite->setPosition(ccp(win.width - 10.f - cropW * k * 0.5f, win.height * 0.5f));
        f.previewSprite->setVisible(true);

        ++f.previewFrames;
    }

    void resetLevel() {
        // Finish the current attempt before resetting per-attempt state.
        PlayLayer::resetLevel();
        m_fields->tracker.reset();
    }

    void onQuit() {
        PlayLayer::onQuit();
    }
};
