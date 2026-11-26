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
        // Preview-only subtree: hitboxes and ground lines, no sprites or background.
        // Hidden in the scene; visible only while the preview renders it.
        CCNode* previewRoot = nullptr;
        CCDrawNode* previewDraw = nullptr;
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

        // Hitboxes only: rebuild the preview subtree every frame from the same
        // hitbox drawer the scene uses, plus ground edges as lines.
        if (!f.previewRoot) {
            f.previewRoot = CCNode::create();
            f.previewDraw = CCDrawNode::create();
            if (!f.previewRoot || !f.previewDraw) return;
            f.previewRoot->addChild(f.previewDraw);
            f.previewRoot->setVisible(false);
            m_objectLayer->addChild(f.previewRoot, 10002);
        }
        f.previewDraw->clear();
        overlay::drawHitboxes(f.previewDraw, m_objects, m_objectLayer, true, true);
        const float halfWidth = win.width / worldToPointScale;
        const ccColor4F groundColour = ccc4f(1.f, 1.f, 1.f, 0.9f);
        // Ground edges: mode 0 is the floor and mode 1 the ceiling. Assumed from
        // the bindings name; check on the phone that they land on the ground.
        for (int mode = 0; mode < 2; ++mode) {
            const float groundY = this->getGroundHeightForMode(mode);
            f.previewDraw->drawSegment(ccp(focus.x - halfWidth, groundY),
                                       ccp(focus.x + halfWidth, groundY), 2.f, groundColour);
        }

        f.previewRoot->setVisible(true);
        const bool rendered = f.portraitTest.render(f.previewRoot, focus, worldToPointScale);
        f.previewRoot->setVisible(false);
        if (!rendered) return;

        const float previewScale = std::min(
            (win.width * 0.42f) / static_cast<float>(f.portraitTest.pointWidth()),
            (win.height * 0.78f) / static_cast<float>(f.portraitTest.pointHeight()));
        portraitTarget->setScale(previewScale);
        // Left edge, vertically centred: keeps the top-right game buttons clear.
        portraitTarget->setPosition(ccp(
            10.f + f.portraitTest.pointWidth() * previewScale * 0.5f,
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
