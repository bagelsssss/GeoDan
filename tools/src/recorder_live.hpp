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

class LiveRecorder final {
public:
    LiveRecorder(); // defined in the .cpp: Session is incomplete here
    LiveRecorder(const LiveRecorder&) = delete;
    LiveRecorder& operator=(const LiveRecorder&) = delete;
    ~LiveRecorder();

    // Called once per frame from PlayLayer::postUpdate, after the overlays
    // have been updated so the recording includes them.
    void update(PlayLayer* layer, bool enabled);

    // Finishes the current session, if any. Blocks until queued frames are
    // written. Safe to call when no session is running.
    void stop(const std::string& reason);

    bool active() const { return m_session != nullptr; }

private:
    struct Session;
    bool start(PlayLayer* layer);
    void captureFrame(PlayLayer* layer);

    std::unique_ptr<Session> m_session;
    bool m_startFailed = false; // cleared when the attempt ends (stop)
};

} // namespace geodan::tools::recorder_live
