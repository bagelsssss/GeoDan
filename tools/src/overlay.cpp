// GeoDan Tools - gameplay overlay. Draws object hitboxes and the player's
// hitbox trail on top of the level. Diagnostic only: it reads game state and
// never changes collision, physics or level objects.
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/GJGroundLayer.hpp>
#include <Geode/binding/RingObject.hpp>

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

#include "core/hitbox_model.hpp"
#include "core/hitbox_trail.hpp"
#include "overlay_renderer.hpp"
#include "portrait_capture_target.hpp"
#include "recorder_live.hpp"

using namespace geode::prelude;
namespace hb = geodan::core::hitbox;
namespace tr = geodan::core::trail;
namespace overlay = geodan::tools::overlay;
namespace capture = geodan::tools::capture;
namespace recorder_live = geodan::tools::recorder_live;

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
        recorder_live::LiveRecorder recorder;
        CCLabelBMFont* recorderStatus = nullptr;
        // Preview-only subtree: hitboxes only, no sprites or background.
        // Hidden in the scene; visible only while it is being rendered.
        CCNode* previewRoot = nullptr;
        CCDrawNode* previewDraw = nullptr;
        // On-screen status for the preview: state of the last update and a count
        // of successful renders. Shows what the preview is doing without logcat.
        CCLabelBMFont* previewStatus = nullptr;
        int previewRenders = 0;
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

        // The preview scene is built whenever the preview or the recorder needs it,
        // so both draw the same hitboxes, ground lines and trail.
        const bool recordingOn = mod->getSettingValue<bool>("level-recorder");
        const bool portraitPreview = mod->getSettingValue<bool>("portrait-preview-test");
        const auto scene = buildScene(portraitPreview, recordingOn || portraitPreview);

        // Level recorder: runs after the scene so each frame matches this tick.
        f.recorder.update(this, recordingOn, scene ? &*scene : nullptr);

        // On-screen recorder status (top-left) so drops can be seen on the phone.
        if (recordingOn) {
            if (!f.recorderStatus) {
                f.recorderStatus = CCLabelBMFont::create("REC", "chatFont.fnt");
                if (f.recorderStatus) {
                    f.recorderStatus->setScale(0.4f);
                    f.recorderStatus->setAnchorPoint(ccp(0.f, 1.f));
                    f.recorderStatus->setColor(ccc3(255, 90, 90));
                    this->addChild(f.recorderStatus, 100001);
                }
            }
            if (f.recorderStatus) {
                const auto winSize = CCDirector::sharedDirector()->getWinSize();
                f.recorderStatus->setVisible(true);
                f.recorderStatus->setPosition(ccp(4.f, winSize.height - 4.f));
                f.recorderStatus->setString(f.recorder.status().c_str());
            }
        } else if (f.recorderStatus) {
            f.recorderStatus->setVisible(false);
        }
    }

    // Builds this tick's preview scene (hitboxes, ground lines, player rectangles
    // and trail) in the preview subtree. showOnScreen also renders it into the
    // on-screen preview; wantScene is for the recorder, which renders the subtree
    // itself. Returns nothing when there is no scene to use this tick.
    std::optional<recorder_live::Scene> buildScene(bool showOnScreen, bool wantScene) {
        auto& f = *m_fields.self();
        const bool showTrail = Mod::get()->getSettingValue<bool>("hitbox-trail");

        auto* portraitTarget = f.portraitTest.renderTexture();
        if (!wantScene) {
            if (portraitTarget) portraitTarget->setVisible(false);
            if (f.previewStatus) f.previewStatus->setVisible(false);
            return std::nullopt;
        }
        if (!showOnScreen) {
            if (portraitTarget) portraitTarget->setVisible(false);
            if (f.previewStatus) f.previewStatus->setVisible(false);
        }
        if (!f.portraitTestInitAttempted) {
            f.portraitTestInitAttempted = true;
            f.portraitTestInitialized = f.portraitTest.initialize(360, 1080);
            if (!f.portraitTestInitialized) {
                log::error("GeoDan portrait preview: failed to create exact 360x1080 render target");
                return std::nullopt;
            }
            portraitTarget = f.portraitTest.renderTexture();
            if (!portraitTarget) return std::nullopt;
            this->addChild(portraitTarget, 100000);
            // Hidden until the first successful render, so an early return never
            // leaves an empty target drawn at the origin.
            portraitTarget->setVisible(false);
            // On-screen status line, bottom-left, so the preview can be diagnosed on the phone.
            f.previewStatus = CCLabelBMFont::create("PV start", "chatFont.fnt");
            if (f.previewStatus) {
                f.previewStatus->setScale(0.4f);
                f.previewStatus->setAnchorPoint(ccp(0.f, 0.f));
                f.previewStatus->setPosition(ccp(4.f, 4.f));
                f.previewStatus->setColor(ccc3(255, 230, 80));
                this->addChild(f.previewStatus, 100001);
            }
        }
        if (!f.portraitTestInitialized || !portraitTarget) return std::nullopt;
        auto setStatus = [&f, showOnScreen](const char* state) {
            if (!showOnScreen || !f.previewStatus) return;
            f.previewStatus->setVisible(true);
            f.previewStatus->setString(
                (std::string("PV ") + state + " " + std::to_string(f.previewRenders)).c_str());
        };
        const auto focus1 = capture::PortraitCaptureTarget::worldPositionOf(m_player1);
        if (!focus1) {
            setStatus("no-player");
            portraitTarget->setVisible(false);
            return std::nullopt;
        }
        const auto win = CCDirector::sharedDirector()->getWinSize();
        if (win.height <= 0.f || f.portraitTest.pointHeight() <= 0) {
            setStatus("bad-size");
            return std::nullopt;
        }

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
            if (!f.previewRoot || !f.previewDraw) {
                setStatus("no-preview");
                return std::nullopt;
            }
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

        // Ground lines: the game's own ground layers (the floor, and the second
        // ground in dual or ship modes), converted through each layer's transform.
        // Same method as the screenshot marker, so the line follows the real ground.
        constexpr float kGroundHalfExtent = 10000.f;
        const ccColor4F groundColour = ccc4f(1.f, 1.f, 1.f, 1.f);
        for (auto* ground : {m_groundLayer, m_groundLayer2}) {
            if (!ground || !ground->isVisible()) continue;
            const float groundY = ground->getGroundY();
            const CCPoint a = m_objectLayer->convertToNodeSpace(
                ground->convertToWorldSpace(ccp(-kGroundHalfExtent, groundY)));
            const CCPoint b = m_objectLayer->convertToNodeSpace(
                ground->convertToWorldSpace(ccp(kGroundHalfExtent, groundY)));
            f.previewDraw->drawSegment(a, b, 2.f, groundColour);
        }

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
        // The hitbox trail follows the same setting as the main overlay.
        if (showTrail) {
            overlay::drawTrail(f.previewDraw, f.tracker.trail(0));
            overlay::drawTrail(f.previewDraw, f.tracker.trail(1));
        }

        if (showOnScreen) {
            f.previewRoot->setVisible(true);
            const bool rendered = f.portraitTest.render(f.previewRoot, focus, worldToPointScale);
            f.previewRoot->setVisible(false);
            if (!rendered) {
                setStatus("render-fail");
            } else {
                ++f.previewRenders;
                setStatus("ok");

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
        }

        return recorder_live::Scene{f.previewRoot, focus, worldToPointScale};
    }

    void resetLevel() {
        // Finish the current attempt before resetting per-attempt state.
        // A restart ends the current recording attempt; the next frame starts a new one.
        m_fields->recorder.stop("level_restart");
        PlayLayer::resetLevel();
        m_fields->tracker.reset();
    }

    void onQuit() {
        m_fields->recorder.stop("user_quit");
        PlayLayer::onQuit();
    }

};

// Button presses and releases are reported on GJBaseGameLayer, where
// handleButton is declared. The game's own handling runs first and is not
// changed; this only records while a level recording is running.
class $modify(GeoDanInputGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        if (auto* recorder = recorder_live::activeRecorder())
            recorder->recordInput(down, button, isPlayer1);
    }

    // Orbs are reported through playerTouchedRing. The recorder keeps only the
    // kinds it knows. Pads and portals are not hooked yet (see the touch log notes).
    void playerTouchedRing(PlayerObject* player, RingObject* object) {
        GJBaseGameLayer::playerTouchedRing(player, object);
        if (auto* recorder = recorder_live::activeRecorder(); recorder && object)
            recorder->recordTouch(static_cast<int>(object->getType()), player == m_player1);
    }
};

