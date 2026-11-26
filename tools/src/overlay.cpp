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
#include "overlay_renderer.hpp"
#include "portrait_capture_target.hpp"
#include "trajectory.hpp"

using namespace geode::prelude;
namespace hb = geodan::core::hitbox;
namespace tr = geodan::core::trail;
namespace overlay = geodan::tools::overlay;
namespace capture = geodan::tools::capture;

namespace {

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
        capture::PortraitCaptureTarget portraitTest;
        bool portraitTestInitialized = false;
        bool portraitTestInitAttempted = false;
        // Preview-only subtree: hitboxes only, no sprites or background.
        // Hidden in the scene; visible only while the preview renders it.
        CCNode* previewRoot = nullptr;
        CCDrawNode* previewDraw = nullptr;
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
        updateGeoDanTrajectory(this, dt);

        // Temporary visual validation only: one extra render pass, no readback or files.
        const bool portraitPreview = mod->getSettingValue<bool>("portrait-preview-test");
        auto* portraitTarget = f.portraitTest.renderTexture();
        if (!portraitPreview) {
            if (portraitTarget) portraitTarget->setVisible(false);
            return;
        }
        if (!f.portraitTestInitAttempted) {
            f.portraitTestInitAttempted = true;
            f.portraitTestInitialized = f.portraitTest.initialize(360, 1080);
            if (!f.portraitTestInitialized) {
                log::error("GeoDan portrait preview: failed to create exact 360x1080 render target");
                return;
            }
            portraitTarget = f.portraitTest.renderTexture();
            if (!portraitTarget) return;
            this->addChild(portraitTarget, 100000);
        }
        if (!f.portraitTestInitialized || !portraitTarget) return;
        const auto focus1 = capture::PortraitCaptureTarget::worldPositionOf(m_player1);
        if (!focus1) { portraitTarget->setVisible(false); return; }
        const auto win = CCDirector::sharedDirector()->getWinSize();
        if (win.height <= 0.f || f.portraitTest.pointHeight() <= 0) return;

        // Dual mode: centre on the middle of both players, and widen the view
        // when they are far apart vertically so both stay visible.
        CCPoint focus = *focus1;
        float span = 0.f;
        if (m_gameState.m_isDualMode && m_player2) {
            if (const auto focus2 = capture::PortraitCaptureTarget::worldPositionOf(m_player2)) {
                focus = ccp((focus1->x + focus2->x) * 0.5f, (focus1->y + focus2->y) * 0.5f);
                span = focus1->y > focus2->y ? focus1->y - focus2->y : focus2->y - focus1->y;
            }
        }
        // Zoomed out from the screen scale (was 1x of the window height).
        const float viewHeight = std::max(win.height * 2.f, span * 1.5f + 200.f);
        const float worldToPointScale = static_cast<float>(f.portraitTest.pointHeight()) / viewHeight;

        // Hitboxes only: level objects and both players, culled to the preview's
        // own view (not the main camera's), in the same coordinates as the scene.
        if (!f.previewRoot) {
            f.previewRoot = CCNode::create();
            f.previewDraw = CCDrawNode::create();
            if (!f.previewRoot || !f.previewDraw) return;
            f.previewRoot->addChild(f.previewDraw);
            f.previewRoot->setVisible(false);
            m_objectLayer->addChild(f.previewRoot, 10002);
        }
        f.previewDraw->clear();
        const float halfWidth = f.portraitTest.pointWidth() / worldToPointScale * 0.5f + 120.f;
        const float halfHeight = f.portraitTest.pointHeight() / worldToPointScale * 0.5f + 120.f;
        const CCPoint centre = m_objectLayer->convertToNodeSpace(focus);
        const CCRect cull = CCRectMake(centre.x - halfWidth, centre.y - halfHeight,
                                       halfWidth * 2.f, halfHeight * 2.f);
        overlay::drawHitboxes(f.previewDraw, m_objects, m_objectLayer, true, true, &cull);

        auto drawPlayerRect = [&](PlayerObject* player, const ccColor4F& colour) {
            if (!player || !player->isVisible()) return;
            const auto rect = player->getObjectRect();
            const CCPoint a = ccp(rect.getMinX(), rect.getMinY());
            const CCPoint b = ccp(rect.getMinX(), rect.getMaxY());
            const CCPoint c = ccp(rect.getMaxX(), rect.getMaxY());
            const CCPoint d = ccp(rect.getMaxX(), rect.getMinY());
            f.previewDraw->drawSegment(a, b, 1.5f, colour);
            f.previewDraw->drawSegment(b, c, 1.5f, colour);
            f.previewDraw->drawSegment(c, d, 1.5f, colour);
            f.previewDraw->drawSegment(d, a, 1.5f, colour);
        };
        drawPlayerRect(m_player1, ccc4f(1.f, 0.2f, 0.2f, 1.f));
        if (m_gameState.m_isDualMode) drawPlayerRect(m_player2, ccc4f(0.2f, 1.f, 0.2f, 1.f));

        f.previewRoot->setVisible(true);
        const bool rendered = f.portraitTest.render(f.previewRoot, focus, worldToPointScale);
        f.previewRoot->setVisible(false);
        if (!rendered) {
            setGeoDanPreviewStatus(f.previewFrames, false);
            return;
        }
        ++f.previewFrames;
        setGeoDanPreviewStatus(f.previewFrames, true);

        const float previewScale = std::min(
            (win.width * 0.42f) / static_cast<float>(f.portraitTest.pointWidth()),
            (win.height * 0.78f) / static_cast<float>(f.portraitTest.pointHeight()));
        portraitTarget->setScale(previewScale);
        // Right edge, vertically centred, clear of the top-right game buttons.
        portraitTarget->setPosition(ccp(
            win.width - 10.f - f.portraitTest.pointWidth() * previewScale * 0.5f,
            win.height * 0.5f));
        portraitTarget->setVisible(true);
    }

    void resetLevel() {
        // Finish the current attempt before resetting per-attempt state.
        PlayLayer::resetLevel();
        resetGeoDanTrajectory(this);
        m_fields->tracker.reset();
    }

    void onQuit() {
        cleanupGeoDanTrajectory(this);
        PlayLayer::onQuit();
    }
};
