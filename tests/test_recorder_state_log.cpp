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
    StateSample dual;
    dual.sequence = 2;
    dual.worldX = 5.5;
    dual.worldY = 6.0;
    dual.hasPlayer2 = true;
    dual.mode2 = PlayerMode::Ship;
    dual.worldX2 = 10.5;
    dual.worldY2 = 20.0;
    samples.push_back(dual);
    assert(stateCsv(samples) ==
           "sequence,mode,upside_down,gravity,world_x,world_y,mode_p2,world_x_p2,world_y_p2\n"
           "0,cube,0,1,0,0,,,\n"
           "1,ship,1,-1,0,0,,,\n"
           "2,cube,0,1,5.5,6,ship,10.5,20\n");
    assert(stateCsv({}) ==
           "sequence,mode,upside_down,gravity,world_x,world_y,mode_p2,world_x_p2,world_y_p2\n");

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

    // Contacts: repeated calls for one object within the gap are one contact.
    ContactFilter filter(100);
    int keyA = 0, keyB = 0;
    assert(filter.newContact(&keyA, 1000));    // first touch starts a contact
    assert(!filter.newContact(&keyA, 1050));   // same contact, within the gap
    assert(!filter.newContact(&keyA, 1100));
    assert(filter.newContact(&keyB, 1100));    // another object is its own contact
    assert(filter.newContact(&keyA, 1300));    // a gap of more than 100 ns ends the contact

    std::puts("Recorder state log tests passed");
    return 0;
}
