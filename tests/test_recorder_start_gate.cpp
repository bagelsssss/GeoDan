// StartGate: when a new recording session may start.
#include "recorder_start_gate.hpp"

#include <cassert>
#include <cstdio>

using geodan::core::recorder::StartGate;

int main() {
    StartGate gate;
    // A fresh gate starts when there is a player and no session.
    assert(gate.mayStart(false, true));
    assert(!gate.mayStart(false, false));  // no player yet
    assert(!gate.mayStart(true, true));    // a session is already running

    // A failed start blocks retries until the attempt ends.
    gate.startFailed();
    assert(gate.failed() && !gate.mayStart(false, true));
    gate.sessionStopped();
    assert(!gate.failed() && gate.mayStart(false, true));

    // Level complete: the session ends and no new one starts on the end screen,
    // however many frames pass and even after stop clears a failure.
    gate.levelCompleted();
    gate.sessionStopped();
    assert(gate.finished());
    for (int frame = 0; frame < 1000; ++frame) assert(!gate.mayStart(false, true));

    // Restarting the level allows the next attempt, and clears a stale failure.
    gate.startFailed();
    gate.levelRestarted();
    assert(!gate.finished() && !gate.failed());
    assert(gate.mayStart(false, true));

    std::puts("Recorder start gate tests passed");
    return 0;
}
