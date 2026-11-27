// GeoDan Level Recorder - live capture bridge (game-thread side).
// One LiveRecorder lives inside each PlayLayer. It owns at most one recording
// session at a time. Capture runs on the game thread; PNG encoding and disk
// writes run on RecorderWorker's own thread. Diagnostic only: it reads game
// state and never changes gameplay, collision or level objects.
#pragma once

#include <Geode/Geode.hpp>

#include <memory>
#include <string>

namespace geodan::tools::recorder_live {

// The preview scene the overlay built this tick: the hitbox-only subtree, the
// focus point and the zoom it was drawn with. A recording renders this same
// subtree, so the frames match the on-screen preview.
struct Scene {
    cocos2d::CCNode* root = nullptr;
    cocos2d::CCPoint focus{};
    float worldToPointScale = 0.f;
};

class LiveRecorder;
// The recorder whose session is running right now, or null. Set by start(),
// cleared by stop(). Lets the GJBaseGameLayer button hook reach it.
LiveRecorder* activeRecorder();

class LiveRecorder final {
public:
    LiveRecorder(); // defined in the .cpp: Session is incomplete here
    LiveRecorder(const LiveRecorder&) = delete;
    LiveRecorder& operator=(const LiveRecorder&) = delete;
    ~LiveRecorder();

    // Called once per frame from PlayLayer::postUpdate, after the preview scene
    // for this tick is built. scene is null when there is no scene this tick.
    void update(PlayLayer* layer, bool enabled, const Scene* scene);

    // Finishes the current session, if any. Blocks until queued frames are
    // written. Safe to call when no session is running.
    void stop(const std::string& reason);

    bool active() const { return m_session != nullptr; }

    // Called from the game's button handler for every press and release. Does
    // nothing when no session is running. Written to inputs.csv when the
    // session stops. Game thread only.
    void recordInput(bool down, int button, bool player1);

    // Called when the player touches an object. Only orbs, pads and portals are
    // kept (see core/recorder_state_log.hpp). Written to touches.csv on stop.
    void recordTouch(int objectType, bool player1);

    // One-line status for the on-screen label: frames handed to the writer,
    // and the count and reason of each drop. Game thread only.
    std::string status() const;

private:
    struct Session;
    bool start(PlayLayer* layer);
    void captureFrame(PlayLayer* layer, const Scene& scene);

    std::unique_ptr<Session> m_session;
    bool m_startFailed = false; // cleared when the attempt ends (stop)
};

} // namespace geodan::tools::recorder_live
