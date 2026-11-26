// GeoDan Tools - gameplay overlay. Draws object hitboxes and the player's
// hitbox trail on top of the level. Diagnostic only: it reads game state and
// never changes collision, physics or level objects.
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/GJGroundLayer.hpp>

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

struct PortraitView {
    CCPoint focus;
    float worldToPointScale = 0.f;
};

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
        // Hidden in the scene; visible only while the preview renders it.
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
        const bool recordingOn = mod->getSettingValue<bool>("level-recorder");
        const bool portraitPreview = mod->getSettingValue<bool>("portrait-preview-test");

        if (!m_objectLayer) {
            f.recorder.update(this, recordingOn, {});
            return;
        }

        if (!f.overlay) {
            f.overlay = CCDrawNode::create();
            if (!f.overlay) {
                f.recorder.update(this, recordingOn, {});
                return;
            }
            m_objectLayer->addChild(f.overlay, 9999);
        }
        if (!f.trailNode) {
            f.trailNode = CCDrawNode::create();
            if (!f.trailNode) {
                f.recorder.update(this, recordingOn, {});
                return;
            }
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

        // The preview and Recorder share one diagnostic-only scene tree. The
        // real m_objectLayer is never rendered into the recording target.
        auto* portraitTarget = f.portraitTest.renderTexture();
        if (!portraitPreview) {
            if (portraitTarget) portraitTarget->setVisible(false);
            if (f.previewStatus) f.previewStatus->setVisible(false);
        } else {
            if (!f.previewStatus) {
                f.previewStatus = CCLabelBMFont::create("PV start", "chatFont.fnt");
                if (f.previewStatus) {
                    f.previewStatus->setScale(0.4f);
                    f.previewStatus->setAnchorPoint(ccp(0.f, 0.f));
                    f.previewStatus->setPosition(ccp(4.f, 4.f));
                    f.previewStatus->setColor(ccc3(255, 230, 80));
                    this->addChild(f.previewStatus, 100001);
                }
            }
            if (!f.portraitTestInitAttempted) {
                f.portraitTestInitAttempted = true;
                f.portraitTestInitialized = f.portraitTest.initialize(360, 1080);
                if (!f.portraitTestInitialized) {
                    log::error("GeoDan portrait preview: failed to create exact 360x1080 render target");
                } else {
                    portraitTarget = f.portraitTest.renderTexture();
                    if (portraitTarget) {
                        this->addChild(portraitTarget, 100000);
                        // Hidden until the first successful render.
                        portraitTarget->setVisible(false);
                    }
                }
            }
        }

        auto setPreviewStatus = [&f](const char* state) {
            if (!f.previewStatus) return;
            f.previewStatus->setVisible(true);
            f.previewStatus->setString(
                (std::string("PV ") + state + " " + std::to_string(f.previewRenders)).c_str());
        };

        std::optional<PortraitView> view;
        if (recordingOn || portraitPreview)
            view = preparePortraitContent(showTrail);

        bool previewRendered = false;
        if (view) {
            // Keep the root hidden in the live game. It is made visible only
            // while either offscreen target visits it.
            f.previewRoot->setVisible(true);
            const recorder_live::CaptureView captureView{
                f.previewRoot, view->focus, view->worldToPointScale
            };
            f.recorder.update(this, recordingOn, captureView);

            if (portraitPreview && f.portraitTestInitialized && portraitTarget) {
                previewRendered = f.portraitTest.render(
                    f.previewRoot, view->focus, view->worldToPointScale);
            }
            f.previewRoot->setVisible(false);
        } else {
            f.recorder.update(this, recordingOn, {});
        }

        if (portraitPreview) {
            if (!f.portraitTestInitialized || !portraitTarget) {
                setPreviewStatus("init-fail");
                if (portraitTarget) portraitTarget->setVisible(false);
            } else if (!view) {
                setPreviewStatus(m_player1 ? "bad-size" : "no-player");
                portraitTarget->setVisible(false);
            } else if (!previewRendered) {
                setPreviewStatus("render-fail");
                portraitTarget->setVisible(false);
            } else {
                ++f.previewRenders;
                setPreviewStatus("ok");

                const auto win = CCDirector::sharedDirector()->getWinSize();
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

        // On-screen Recorder status: counts and last drop reason are diagnostic
        // UI only and are not part of the shared portrait scene.
        if (recordingOn) {
            if (!f.recorderStatus) {
                f.recorderStatus = CCLabelBMFont::create("REC", "chatFont.fnt");
                if (f.recorderStatus) {
                    f.recorderStatus->setScale(0.4f);
                    f.recorderStatus->setAnchorPoint(ccp(0.f, 1.f));
                    f.recorderStatus->setColor(ccc3(255, 90, 90));
                    this->addChild(f.recorderStatus, 100002);
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

    std::optional<PortraitView> preparePortraitContent(bool showTrail) {
        auto& f = *m_fields.self();
        if (!m_player1 || !m_objectLayer) return std::nullopt;

        const auto focus1 = capture::PortraitCaptureTarget::worldPositionOf(m_player1);
        if (!focus1) return std::nullopt;

        const auto win = CCDirector::sharedDirector()->getWinSize();
        const float contentScale = CC_CONTENT_SCALE_FACTOR();
        if (win.height <= 0.f || !std::isfinite(contentScale) || contentScale <= 0.f)
            return std::nullopt;

        // These point dimensions represent the exact 360x1080 output pixels,
        // whether the visible preview target has been initialized or not.
        const float pointWidth = 360.f / contentScale;
        const float pointHeight = 1080.f / contentScale;

        // Dual mode: centre on the middle of both players, and widen the view
        // when they are far apart vertically so both stay visible.
        CCPoint focus = *focus1;
        float span = 0.f;
        if (m_gameState.m_isDualMode && m_player2) {
            if (const auto focus2 = capture::PortraitCaptureTarget::worldPositionOf(m_player2)) {
                focus = ccp((focus1->x + focus2->x) * 0.5f,
                            (focus1->y + focus2->y) * 0.5f);
                span = std::abs(focus1->y - focus2->y);
            }
        }

        const float viewHeight = std::max(win.height * 2.f, span * 1.5f + 200.f);
        const float worldToPointScale = pointHeight / viewHeight;
        if (!std::isfinite(worldToPointScale) || worldToPointScale <= 0.f)
            return std::nullopt;

        // A dedicated subtree: collision outlines, ground, player rectangles,
        // and the optional trail. No icon sprites, object art, or background.
        if (!f.previewRoot) {
            f.previewRoot = CCNode::create();
            f.previewDraw = CCDrawNode::create();
            if (!f.previewRoot || !f.previewDraw) return std::nullopt;
            f.previewRoot->addChild(f.previewDraw);
            f.previewRoot->setVisible(false);
            m_objectLayer->addChild(f.previewRoot, 10002);
        }

        f.previewDraw->clear();
        const float halfWidth = pointWidth / worldToPointScale * 0.5f + 120.f;
        const float halfHeight = pointHeight / worldToPointScale * 0.5f + 120.f;
        const CCPoint centre = m_objectLayer->convertToNodeSpace(focus);
        const CCRect cull = CCRectMake(centre.x - halfWidth, centre.y - halfHeight,
                                       halfWidth * 2.f, halfHeight * 2.f);
        overlay::drawHitboxes(f.previewDraw, m_objects, m_objectLayer, true, true, &cull);

        // Use both actual game ground layers, converted through their transforms.
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
        if (m_gameState.m_isDualMode)
            drawPlayerRect(m_player2, ccc4f(0.2f, 1.f, 0.2f, 1.f));

        if (showTrail) {
            overlay::drawTrail(f.previewDraw, f.tracker.trail(0));
            overlay::drawTrail(f.previewDraw, f.tracker.trail(1));
        }

        return PortraitView{focus, worldToPointScale};
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
