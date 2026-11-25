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
        const auto focus = capture::PortraitCaptureTarget::worldPositionOf(m_player1);
        if (!focus) { portraitTarget->setVisible(false); return; }
        const auto win = CCDirector::sharedDirector()->getWinSize();
        if (win.height <= 0.f || f.portraitTest.pointHeight() <= 0) return;
        const float worldToPointScale = static_cast<float>(f.portraitTest.pointHeight()) / win.height;
        if (!f.portraitTest.render(m_objectLayer, *focus, worldToPointScale)) return;
        const float previewScale = std::min(
            (win.width * 0.42f) / static_cast<float>(f.portraitTest.pointWidth()),
            (win.height * 0.78f) / static_cast<float>(f.portraitTest.pointHeight()));
        portraitTarget->setScale(previewScale);
        portraitTarget->setPosition(ccp(
            win.width - f.portraitTest.pointWidth() * previewScale * 0.5f - 10.f,
            win.height - f.portraitTest.pointHeight() * previewScale * 0.5f - 10.f));
        portraitTarget->setVisible(true);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->tracker.reset();
    }
};
