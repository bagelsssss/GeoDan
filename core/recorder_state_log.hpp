// GeoDan Level Recorder - player state and touch logs (portable).
// states.csv has one row per captured frame: the player's mode and orientation.
// touches.csv has one row per orb, pad or portal the player touched.
// Object type numbers are GameObjectType values from the 2.2081 bindings.
#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace geodan::core::recorder {

enum class PlayerMode { Cube, Ship, Ball, Ufo, Wave, Robot, Spider, Swing };

// The PlayerObject flags the game sets for each mode.
struct PlayerFlags {
    bool ship = false;   // m_isShip
    bool ufo = false;    // m_isBird
    bool ball = false;   // m_isBall
    bool wave = false;   // m_isDart
    bool robot = false;  // m_isRobot
    bool spider = false; // m_isSpider
    bool swing = false;  // m_isSwing
};

// Cube when no mode flag is set.
inline PlayerMode modeFromFlags(const PlayerFlags& f) {
    if (f.ship) return PlayerMode::Ship;
    if (f.ufo) return PlayerMode::Ufo;
    if (f.ball) return PlayerMode::Ball;
    if (f.wave) return PlayerMode::Wave;
    if (f.robot) return PlayerMode::Robot;
    if (f.spider) return PlayerMode::Spider;
    if (f.swing) return PlayerMode::Swing;
    return PlayerMode::Cube;
}

inline const char* modeName(PlayerMode mode) {
    switch (mode) {
        case PlayerMode::Cube: return "cube";
        case PlayerMode::Ship: return "ship";
        case PlayerMode::Ball: return "ball";
        case PlayerMode::Ufo: return "ufo";
        case PlayerMode::Wave: return "wave";
        case PlayerMode::Robot: return "robot";
        case PlayerMode::Spider: return "spider";
        case PlayerMode::Swing: return "swing";
    }
    return "cube";
}

struct StateSample {
    std::uint64_t sequence = 0;
    PlayerMode mode = PlayerMode::Cube;
    bool upsideDown = false;
    double gravity = 1.0; // PlayerObject::m_gravity
};

// CSV text: header, then one row per sample. Columns: sequence,mode,upside_down,gravity.
inline std::string stateCsv(const std::vector<StateSample>& samples) {
    std::ostringstream out;
    out << "sequence,mode,upside_down,gravity\n";
    for (const auto& s : samples)
        out << s.sequence << ',' << modeName(s.mode) << ',' << (s.upsideDown ? 1 : 0) << ','
            << s.gravity << '\n';
    return out.str();
}

enum class TouchKind { Orb, Pad, Portal };

// Maps a GameObjectType value to a touch kind. Returns false for other objects.
inline bool touchKindFor(int objectType, TouchKind& kind) {
    switch (objectType) {
        // Orbs: yellow, pink, gravity, green, drop, red, custom, dash, gravity dash, spider, teleport.
        case 11: case 12: case 13: case 29: case 32: case 35: case 36: case 37: case 38:
        case 43: case 46:
            kind = TouchKind::Orb;
            return true;
        // Pads: yellow, pink, gravity, red, spider.
        case 8: case 9: case 10: case 34: case 44:
            kind = TouchKind::Pad;
            return true;
        // Portals: gravity, ship, cube, mirror, ball, size, ufo, dual, solo, wave, robot,
        // teleport, spider, swing, gravity toggle.
        case 3: case 4: case 5: case 6: case 14: case 15: case 16: case 17: case 18: case 19:
        case 23: case 24: case 26: case 27: case 28: case 33: case 41: case 42:
            kind = TouchKind::Portal;
            return true;
        default:
            return false;
    }
}

inline const char* touchKindName(TouchKind kind) {
    switch (kind) {
        case TouchKind::Orb: return "orb";
        case TouchKind::Pad: return "pad";
        case TouchKind::Portal: return "portal";
    }
    return "orb";
}

struct TouchEvent {
    std::uint64_t timeNs = 0;
    TouchKind kind = TouchKind::Orb;
    int objectType = 0;
    bool player1 = true;
};

// CSV text: time_ns,kind,object_type,player1 (player1 is 1 or 0).
inline std::string touchCsv(const std::vector<TouchEvent>& events) {
    std::ostringstream out;
    out << "time_ns,kind,object_type,player1\n";
    for (const auto& e : events)
        out << e.timeNs << ',' << touchKindName(e.kind) << ',' << e.objectType << ','
            << (e.player1 ? 1 : 0) << '\n';
    return out.str();
}

} // namespace geodan::core::recorder
