// Tests for the I/I Analyzer substage 2 motion check.
#include "core/input_motion_check.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace geodan::core::motion;

namespace {

// One cube sample per frame. x advances 5.2 per frame unless a jitter table says otherwise.
struct Builder {
    std::vector<Sample> track;
    double x = 0.0, y = 100.0, vy = 0.0;
    double dt = 1.0;
    void rest(int frames) { for (int i = 0; i < frames; ++i) push(0.0); }
    void push(double velocity) {
        vy = velocity;
        x += 5.2 * dt;
        y += vy * dt;
        track.push_back({true, true, false, x, y});
    }
    // Free fall: vy decreases by 0.78 per unit time. Mirrors the real cube arc.
    void arc(int frames, double jitter = 1.0) {
        for (int i = 0; i < frames; ++i) {
            dt = (i % 3 == 1) ? jitter : 1.0;
            vy -= 0.78 * dt;
            x += 5.2 * dt;
            y += vy * dt;
            track.push_back({true, true, false, x, y});
        }
        dt = 1.0;
    }
    void jump() { vy = 10.0; }
};

} // namespace

int main() {
    // A clean takeoff from rest: press on frame 5, the player leaves the ground on frame 5.
    {
        Builder b;
        b.rest(5);
        b.jump();
        b.arc(20);
        const auto m = analyzePlayer(b.track, {5}, {}, b.track.size());
        assert(m.takeoffPresses == 1 && m.noTakeoffPresses == 0 && m.unknownPresses == 0);
        assert(m.takeoffOffsets[0] == 1);
        // The jump is explained by the press; the arc after it is predicted exactly.
        assert(m.deviationsByCause[static_cast<int>(Cause::Press)] == 1);
        assert(m.deviationsByCause[static_cast<int>(Cause::Unexplained)] == 0);
    }

    // Uneven frame times do not create deviations, because the arc is predicted against x.
    {
        Builder b;
        b.rest(5);
        b.jump();
        b.arc(24, 1.6);
        const auto m = analyzePlayer(b.track, {5}, {}, b.track.size());
        assert(m.deviationsByCause[static_cast<int>(Cause::Unexplained)] == 0);
    }

    // A press in mid-air (the arc is already moving) is a no_takeoff, not an error.
    {
        Builder b;
        b.rest(5);
        b.jump();
        b.arc(20);
        const auto m = analyzePlayer(b.track, {5, 12}, {}, b.track.size());
        assert(m.takeoffPresses == 1 && m.noTakeoffPresses == 1);
    }

    // A moving block pushes the player up mid-arc with no press and no touch: unexplained.
    {
        Builder b;
        b.rest(5);
        b.jump();
        b.arc(8);
        b.y += 6.0;                // the block lifts the player
        b.track.back().y = b.y;
        b.arc(8);
        const auto m = analyzePlayer(b.track, {5}, {}, b.track.size());
        assert(m.deviationsByCause[static_cast<int>(Cause::Unexplained)] >= 1);
        const auto& d = m.deviationList.back();
        (void)d;
        bool found = false;
        for (const auto& dev : m.deviationList)
            if (dev.frame == 5 + 1 + 8 - 1 + 0 || (dev.cause == Cause::Unexplained && dev.residual > 0)) found = true;
        assert(found);
    }

    // The same lift next to a touch is explained by the touch.
    {
        Builder b;
        b.rest(5);
        b.jump();
        b.arc(8);
        b.y += 6.0;
        b.track.back().y = b.y;
        const std::size_t liftFrame = b.track.size() - 1;
        b.arc(8);
        const auto m = analyzePlayer(b.track, {5}, {liftFrame}, b.track.size());
        assert(m.deviationsByCause[static_cast<int>(Cause::Touch)] >= 1);
    }

    // Landing: the arc heads for the floor and the player stops. That is a contact.
    {
        Builder b;
        b.rest(5);
        b.jump();
        b.arc(26);                  // the arc comes back down through the floor
        b.y = 100.0;                // the floor stops it
        b.track.back().y = b.y;
        b.rest(4);
        const auto m = analyzePlayer(b.track, {5}, {}, b.track.size());
        assert(m.deviationsByCause[static_cast<int>(Cause::Contact)] >= 1);
    }

    // A mirrored player (height moves the other way) takes off the same way.
    {
        Builder b;
        b.rest(5);
        b.vy = -10.0;
        b.y = 345.0;
        b.arc(1);                   // unused: just move the baseline
        b.track.clear();
        b.x = 0.0;
        for (int i = 0; i < 5; ++i) b.push(0.0);
        b.vy = -10.0;
        for (int i = 0; i < 20; ++i) {
            b.vy += 0.78;
            b.x += 5.2;
            b.y += b.vy;
            b.track.push_back({true, true, false, b.x, b.y});
        }
        const auto m = analyzePlayer(b.track, {5}, {}, b.track.size());
        assert(m.takeoffPresses == 1);
    }

    // Unknown: too early for history, after the active region, or in another mode.
    {
        Builder b;
        b.rest(30);
        auto ship = b.track;
        for (auto& s : ship) s.cube = false;
        assert(analyzePlayer(b.track, {1}, {}, b.track.size()).unknownPresses == 1);
        assert(analyzePlayer(b.track, {29}, {}, b.track.size()).unknownPresses == 1);
        assert(analyzePlayer(b.track, {10}, {}, 12).unknownPresses == 1);   // needs frames up to f+3
        assert(analyzePlayer(ship, {10}, {}, ship.size()).unknownPresses == 1);
        assert(analyzePlayer(ship, {10}, {}, ship.size()).deviations == 0);
        // No position data at all.
        std::vector<Sample> none(30);
        assert(analyzePlayer(none, {10}, {}, none.size()).unknownPresses == 1);
    }

    // A stationary player (x not advancing) is not judged.
    {
        std::vector<Sample> still;
        for (int i = 0; i < 10; ++i) still.push_back({true, true, false, 0.0, 100.0 + i * 9.0});
        assert(analyzePlayer(still, {}, {}, still.size()).deviations == 0);
    }

    // states.csv reading: columns found by name, player 2 optional.
    {
        const std::string text =
            "sequence,mode,upside_down,gravity,world_x,world_y,mode_p2,world_x_p2,world_y_p2\n"
            "10,cube,0,0.9,1,100,,,\n"
            "11,cube,0,0.9,2,101,cube,2,300\n"
            "12,ship,1,0.9,3,102,cube,3,299\n";
        const auto tracks = readStateSamples(text, {10, 11, 12});
        assert(tracks);
        assert((*tracks)[0][0].valid && (*tracks)[0][0].cube && (*tracks)[0][0].y == 100.0);
        assert(!(*tracks)[1][0].valid);                 // player 2 absent on frame 0
        assert((*tracks)[1][1].valid && (*tracks)[1][1].y == 300.0);
        assert(!(*tracks)[0][2].cube && (*tracks)[0][2].upsideDown);
        assert(!readStateSamples("sequence,mode\n1,cube\n", {1}));   // no height column
        assert(!readStateSamples("", {}));
        // A sequence with no frame is skipped.
        const auto skipped = readStateSamples(text, {10});
        assert(skipped && (*skipped)[0].size() == 1);
    }

    // touches.csv reading: bad rows skipped.
    {
        const auto t = readTouchTimes("time_ns,kind,object_type,player1\n100,pad,8,1\nbad,orb,1,0\n200,orb,13,0\n");
        assert(t.size() == 2 && t[0].timeNs == 100 && t[0].player1 && t[1].timeNs == 200 && !t[1].player1);
        assert(readTouchTimes("").empty());
    }

    std::puts("Input motion check tests passed");
    return 0;
}
