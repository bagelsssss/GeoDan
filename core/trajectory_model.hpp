// GeoDan Tools - trajectory rules. Pure C++ (no Geode, no Cocos).
// Decides how far a prediction runs, why it ended, when it is refreshed, and
// what the log line says. The game-facing simulation is in tools/src/trajectory.cpp.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace geodan::core::trajectory {

// Fixed physics step: 1/240 s, the game's native tick, regardless of frame time.
inline constexpr float kStepSeconds = 1.f / 240.f;
inline constexpr std::size_t kMinSteps = 1;
inline constexpr std::size_t kMaxSteps = 960;
inline constexpr std::size_t kDefaultSteps = 480;   // about 2 s at 240 Hz
inline constexpr std::uint64_t kRefreshEvery = 8;   // ticks between timed refreshes

enum class StopReason {
    None,               // not simulated yet
    HorizonReached,     // every step ran
    HazardDeath,        // the hidden copy hit a hazard
    UnsupportedTrigger, // pad, orb or portal, with trigger continuation off
    ModeSwitch,         // the copy would change gamemode (not modelled)
    InvalidPosition,    // a non-finite position was produced
    PlayerUnavailable,  // no real player, copy or object layer
};

inline const char* stopReasonName(StopReason r) {
    switch (r) {
        case StopReason::None: return "none";
        case StopReason::HorizonReached: return "horizon_reached";
        case StopReason::HazardDeath: return "hazard_death";
        case StopReason::UnsupportedTrigger: return "unsupported_trigger";
        case StopReason::ModeSwitch: return "mode_switch";
        case StopReason::InvalidPosition: return "invalid_position";
        case StopReason::PlayerUnavailable: return "player_unavailable";
    }
    return "unknown";
}

// Steps from the setting, kept inside [kMinSteps, kMaxSteps].
inline std::size_t clampSteps(long long requested) {
    if (requested < static_cast<long long>(kMinSteps)) return kMinSteps;
    if (requested > static_cast<long long>(kMaxSteps)) return kMaxSteps;
    return static_cast<std::size_t>(requested);
}

struct Point {
    float x = 0.f;
    float y = 0.f;
};

inline constexpr std::size_t kNoIndex = static_cast<std::size_t>(-1);

struct Branch {
    std::vector<Point> points;       // includes the start point
    std::size_t requestedSteps = 0;
    std::size_t completedSteps = 0;
    StopReason stop = StopReason::None;
    int stopObjectID = 0;
    // Points from this index on come after a trigger whose effect is not modelled.
    std::size_t guessFrom = kNoIndex;
    int guessObjectID = 0;
    std::size_t triggersPassed = 0;

    float horizonSeconds() const { return static_cast<float>(completedSteps) * kStepSeconds; }
    bool stoppedEarly() const { return stop != StopReason::HorizonReached; }
};

// What decides whether a prediction is out of date.
struct InputState {
    std::uint32_t modeBits = 0;
    bool holding = false;
    bool grounded = false;
    bool grounded2 = false;
    bool upsideDown = false;
    float speed = 0.f;
    float gravity = 0.f;

    bool operator==(const InputState& o) const {
        return modeBits == o.modeBits && holding == o.holding && grounded == o.grounded &&
               grounded2 == o.grounded2 && upsideDown == o.upsideDown &&
               speed == o.speed && gravity == o.gravity;
    }
};

// Refresh on a state change, or every kRefreshEvery ticks.
inline bool needsRefresh(bool justCreated, bool stateChanged, std::uint64_t tick) {
    return justCreated || stateChanged || tick % kRefreshEvery == 0;
}

// The log line for one branch. Trigger details are added only when there are some.
inline std::string describe(const char* name, const Branch& b) {
    std::string s = std::string(name) + " steps=" + std::to_string(b.completedSteps) + "/" +
                    std::to_string(b.requestedSteps) + " horizon=" +
                    std::to_string(b.horizonSeconds()) + "s stop=" + stopReasonName(b.stop);
    if (b.stopObjectID != 0) s += " object=" + std::to_string(b.stopObjectID);
    if (b.guessFrom != kNoIndex) {
        s += " guess_from=" + std::to_string(b.guessFrom) + " trigger=" +
             std::to_string(b.guessObjectID) + " passed=" + std::to_string(b.triggersPassed);
    }
    return s;
}

}  // namespace geodan::core::trajectory
