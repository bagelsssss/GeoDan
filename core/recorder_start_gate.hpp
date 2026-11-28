// GeoDan Level Recorder - when a new recording session may start.
// The live recorder starts a session on any frame that has a player and no session.
// Two things must stop that: a failed start (try again only after the attempt ends)
// and a finished level (the end screen must not be recorded as a new attempt until
// the level is restarted).
#pragma once

namespace geodan::core::recorder {

class StartGate {
public:
    // True when a session may start on this frame.
    bool mayStart(bool sessionActive, bool playerPresent) const {
        return !sessionActive && playerPresent && !m_startFailed && !m_finished;
    }

    void startFailed() { m_startFailed = true; }

    // Called whenever a session or attempt ends. A failed start may be retried afterwards.
    void sessionStopped() { m_startFailed = false; }

    // The level was completed. No new session starts until levelRestarted().
    void levelCompleted() { m_finished = true; }

    // The level was restarted: the next frame may start a new attempt.
    void levelRestarted() { m_finished = false; m_startFailed = false; }

    bool failed() const { return m_startFailed; }
    bool finished() const { return m_finished; }

private:
    bool m_startFailed = false;
    bool m_finished = false;
};

} // namespace geodan::core::recorder
