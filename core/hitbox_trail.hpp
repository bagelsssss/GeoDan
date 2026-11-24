// GeoDan Tools - hitbox trail history. Pure C++. Keeps the player's recent
// bounds, one sample per physics tick, trimmed by age and by count.
// Ported from the old repo's trajectory.hpp Tracker.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>

namespace geodan::core::trail {

inline constexpr double kDefaultTrailSeconds = 2.5;
inline constexpr double kMinimumTrailSeconds = 0.25;
inline constexpr double kMaximumTrailSeconds = 10.0;
inline constexpr std::size_t kMaximumSamples = 960;

struct Bounds {
    float left = 0.f;
    float bottom = 0.f;
    float right = 0.f;
    float top = 0.f;
};

// One player's bounds this tick. collisionBlock is the smaller inner box.
struct Snapshot {
    Bounds outer;
    std::optional<Bounds> collisionBlock;
};

struct Sample {
    double elapsedSeconds = 0.0;
    Bounds outer;
    std::optional<Bounds> collisionBlock;
};

using Trail = std::deque<Sample>;

class Tracker {
public:
    void reset() {
        m_trails = {};
        m_elapsedSeconds = 0.0;
    }

    void setTrailSeconds(double seconds) {
        if (!std::isfinite(seconds)) return;
        m_trailSeconds = std::clamp(seconds, kMinimumTrailSeconds, kMaximumTrailSeconds);
        for (auto& trail : m_trails) trim(trail);
    }

    // Called once per physics tick. A missing snapshot means that player is not shown.
    void observeTick(float dt, const std::optional<Snapshot>& player1,
                     const std::optional<Snapshot>& player2) {
        if (std::isfinite(dt) && dt > 0.f) m_elapsedSeconds += static_cast<double>(dt);
        append(0, player1);
        append(1, player2);
    }

    const Trail& trail(std::size_t player) const {
        static const Trail empty;
        return player < m_trails.size() ? m_trails[player] : empty;
    }

private:
    void append(std::size_t player, const std::optional<Snapshot>& snapshot) {
        if (!snapshot || player >= m_trails.size()) return;
        auto& trail = m_trails[player];
        trail.push_back(Sample{m_elapsedSeconds, snapshot->outer, snapshot->collisionBlock});
        trim(trail);
    }

    void trim(Trail& trail) const {
        while (trail.size() > 1 &&
               m_elapsedSeconds - trail.front().elapsedSeconds > m_trailSeconds)
            trail.pop_front();
        while (trail.size() > kMaximumSamples) trail.pop_front();
    }

    std::array<Trail, 2> m_trails;
    double m_elapsedSeconds = 0.0;
    double m_trailSeconds = kDefaultTrailSeconds;
};

}  // namespace geodan::core::trail
