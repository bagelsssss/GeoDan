#pragma once

// GeoDan trajectory prediction data model.
// This file deliberately contains no Geometry Dash calls or substitute physics.
// The game-facing adapter must feed it positions produced by the real player
// update/collision routines under a separately audited simulation guard.

#include <array>
#include <cstddef>
#include <vector>

namespace geodan::core::trajectory {

struct Vec2 {
    float x = 0.f;
    float y = 0.f;
};

enum class Branch : std::size_t {
    Hold = 0,
    Release = 1,
};

enum class StopReason {
    None,
    Collision,
    UnsafeInteraction,
    InvalidState,
    StepLimit,
};

struct Sample {
    Vec2 position{};
    float rotation = 0.f;
};

struct Path {
    std::vector<Sample> samples;
    StopReason stopReason = StopReason::None;
    bool complete = false;

    [[nodiscard]] bool stopped() const {
        return stopReason != StopReason::None || complete;
    }

    // Appends a predicted state unless this path has already stopped.
    // The caller owns the physics step and must supply a finite, valid sample.
    bool append(const Sample& sample, std::size_t maxSamples) {
        if (stopped() || maxSamples == 0 || samples.size() >= maxSamples) {
            if (!stopped() && samples.size() >= maxSamples)
                stopReason = StopReason::StepLimit;
            return false;
        }
        samples.push_back(sample);
        if (samples.size() >= maxSamples)
            stopReason = StopReason::StepLimit;
        return true;
    }

    void stop(StopReason reason) {
        if (!stopped() && reason != StopReason::None)
            stopReason = reason;
    }

    void finish() {
        if (!stopped())
            complete = true;
    }

    void reset() {
        samples.clear();
        stopReason = StopReason::None;
        complete = false;
    }
};

struct Settings {
    // Hard safety cap; UI settings may lower this but never raise it.
    static constexpr std::size_t kHardMaxSamples = 600;
    std::size_t maxSamples = 180;

    [[nodiscard]] std::size_t boundedMaxSamples() const {
        if (maxSamples == 0)
            return 1;
        return maxSamples > kHardMaxSamples ? kHardMaxSamples : maxSamples;
    }
};

struct Prediction {
    std::array<Path, 2> paths{};

    [[nodiscard]] Path& path(Branch branch) {
        return paths[static_cast<std::size_t>(branch)];
    }

    [[nodiscard]] const Path& path(Branch branch) const {
        return paths[static_cast<std::size_t>(branch)];
    }

    void reset() {
        for (auto& branch : paths)
            branch.reset();
    }
};

// Shared by game hooks on the gameplay thread. Nested scopes are supported
// so a helper cannot accidentally clear an outer simulation guard.
class SimulationContext {
public:
    class Scope {
    public:
        Scope() { ++depth(); }
        ~Scope() { --depth(); }

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&&) = delete;
        Scope& operator=(Scope&&) = delete;
    };

    [[nodiscard]] static bool active() { return depth() != 0; }
    [[nodiscard]] static std::size_t nestingDepth() { return depth(); }

private:
    static std::size_t& depth() {
        static thread_local std::size_t value = 0;
        return value;
    }
};

} // namespace geodan::core::trajectory
