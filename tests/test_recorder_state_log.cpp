// Portable test for core/recorder_state_log.hpp. Build:
// g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. tests/test_recorder_state_log.cpp
#include "recorder_state_log.hpp"

#include <cassert>
#include <cstdio>

using namespace geodan::core::recorder;

int main() {
    // Mode: a cube unless a mode flag is set.
    assert(modeFromFlags({}) == PlayerMode::Cube);
    PlayerFlags ship;
    ship.ship = true;
    assert(modeFromFlags(ship) == PlayerMode::Ship);
    PlayerFlags wave;
    wave.wave = true;
    assert(modeFromFlags(wave) == PlayerMode::Wave);
    PlayerFlags ufo;
    ufo.ufo = true;
    assert(modeFromFlags(ufo) == PlayerMode::Ufo);

    // State CSV: header, then one row per sample.
    std::vector<StateSample> samples;
    StateSample a;
    a.sequence = 0;
    samples.push_back(a);
    StateSample b;
    b.sequence = 1;
    b.mode = PlayerMode::Ship;
    b.upsideDown = true;
    b.gravity = -1.0;
    samples.push_back(b);
    assert(stateCsv(samples) ==
           "sequence,mode,upside_down,gravity\n"
           "0,cube,0,1\n"
           "1,ship,1,-1\n");
    assert(stateCsv({}) == "sequence,mode,upside_down,gravity\n");

    // Touch kinds: known types map to orb, pad or portal; others are rejected.
    TouchKind kind = TouchKind::Orb;
    assert(touchKindFor(11, kind) && kind == TouchKind::Orb);  // yellow jump ring
    assert(touchKindFor(8, kind) && kind == TouchKind::Pad);   // yellow jump pad
    assert(touchKindFor(3, kind) && kind == TouchKind::Portal); // gravity portal
    assert(!touchKindFor(2, kind));   // hazard
    assert(!touchKindFor(0, kind));   // solid
    assert(!touchKindFor(22, kind));  // secret coin

    // Touch CSV: header, then one row per event.
    std::vector<TouchEvent> touches;
    TouchEvent t;
    t.timeNs = 1234;
    t.kind = TouchKind::Pad;
    t.objectType = 8;
    t.player1 = false;
    touches.push_back(t);
    assert(touchCsv(touches) ==
           "time_ns,kind,object_type,player1\n"
           "1234,pad,8,0\n");

    std::puts("Recorder state log tests passed");
    return 0;
}
