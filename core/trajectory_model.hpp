// GeoDan trajectory predictor: isolated, deterministic first-pass model.
// This model deliberately does not call PlayerObject::update or mutate the live
// GJBaseGameLayer. Unsupported interactions terminate a branch as uncertain.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace geodan::core::trajectory {

inline constexpr float kStepSeconds = 1.0f / 240.0f;
inline constexpr std::size_t kDefaultSteps = 120;

struct Vec2 {
    float x = 0.f;
    float y = 0.f;
};

struct Bounds {
    float left = 0.f;
    float bottom = 0.f;
    float right = 0.f;
    float top = 0.f;
};

inline bool overlaps(const Bounds& a, const Bounds& b) {
    return a.left < b.right && a.right > b.left &&
           a.bottom < b.top && a.top > b.bottom;
}

enum class Mode { Cube, Ship, Ufo, Wave, Swing, Ball, Robot, Spider, Unsupported };
enum class ObjectKind { Solid, Hazard, Interaction, Slope };

enum class StopReason {
    Horizon,
    Hazard,
    SolidContact,
    UnsupportedInteraction,
    UnsupportedMode,
    InvalidState
};

struct Obstacle {
    Bounds bounds;
    ObjectKind kind = ObjectKind::Interaction;
};

struct State {
    Vec2 position;
    float velocityY = 0.f;
    float velocityX = 0.f;
    float gravity = 0.958199f;
    float gravityMultiplier = 1.f;
    float jumpVelocity = 11.180032f;
    float playerSpeed = 0.9f;
    float speedMultiplier = 5.770002f;
    float scale = 1.f;
    float halfWidth = 5.f;
    float halfHeight = 5.f;
    bool upsideDown = false;
    bool grounded = false;
    bool canJump = false;
    bool jumpUnused = true;
    bool jumping = false;
    bool holdingJump = false;
    bool isPlatformer = false;
    bool goingLeft = false;
    bool velocityUncapped = false;
    bool robotBoostInvalidated = false;
    float robotBoostCharge = 0.f;
    Mode mode = Mode::Unsupported;
};

struct Sample {
    Vec2 position;
    float time = 0.f;
};

struct Result {
    std::vector<Sample> samples;
    StopReason reason = StopReason::Horizon;
    bool reliable = true;
};

struct Request {
    std::size_t maxSteps = kDefaultSteps;
    float stepSeconds = kStepSeconds;
};

inline Bounds playerBounds(const State& s) {
    return {s.position.x - s.halfWidth, s.position.y - s.halfHeight,
            s.position.x + s.halfWidth, s.position.y + s.halfHeight};
}

inline float flip(const State& s) { return s.upsideDown ? -1.f : 1.f; }

inline bool valid(const State& s) {
    return std::isfinite(s.position.x) && std::isfinite(s.position.y) &&
           std::isfinite(s.velocityY) && std::isfinite(s.velocityX) &&
           std::isfinite(s.gravity) && std::isfinite(s.gravityMultiplier) &&
           std::isfinite(s.jumpVelocity) && std::isfinite(s.scale) &&
           s.scale > 0.f && s.halfWidth > 0.f && s.halfHeight > 0.f;
}

// One deterministic branch. The input is held constant for the prediction;
// this is a preview, not an input-search or bot-playing routine.
inline Result predict(State state, bool holdJump, const std::vector<Obstacle>& obstacles,
                      Request request = {}) {
    Result out;
    if (!valid(state) || !(request.stepSeconds > 0.f) ||
        !std::isfinite(request.stepSeconds)) {
        out.reason = StopReason::InvalidState;
        out.reliable = false;
        return out;
    }
    if (state.isPlatformer || state.mode == Mode::Spider ||
        state.mode == Mode::Unsupported) {
        out.reason = StopReason::UnsupportedMode;
        out.reliable = false;
        out.samples.push_back({state.position, 0.f});
        return out;
    }

    const float dt = request.stepSeconds;
    const bool flyingMode = state.mode == Mode::Ship || state.mode == Mode::Ufo ||
                            state.mode == Mode::Wave || state.mode == Mode::Swing;
    const float scaleMod = state.scale == 1.f ? 1.f : (flyingMode ? 0.85f : 0.8f);
    const float modifiedGravity = state.gravity * state.gravityMultiplier;
    const float horizontalSpeed = state.playerSpeed * state.speedMultiplier;

    out.samples.reserve(request.maxSteps + 1);
    out.samples.push_back({state.position, 0.f});

    for (std::size_t i = 0; i < request.maxSteps; ++i) {
        const float scaledDt = dt * 0.9f;
        const float gravityFlip = flip(state);

        switch (state.mode) {
            case Mode::Cube:
            case Mode::Robot:
            case Mode::Ball: {
                const float gravityScale = state.mode == Mode::Robot ? 0.9f :
                                           state.mode == Mode::Ball ? 0.6f : 1.f;
                if (holdJump && state.canJump && state.grounded) {
                    float jump = state.jumpVelocity;
                    if (state.mode == Mode::Robot) jump *= 0.5f;
                    state.velocityY = jump * gravityFlip * scaleMod;
                    state.grounded = false;
                    state.canJump = false;
                    state.jumping = true;
                    state.jumpUnused = false;
                    if (state.mode == Mode::Ball) {
                        state.upsideDown = !state.upsideDown;
                        state.velocityY *= 0.6f;
                        holdJump = false;
                    }
                } else {
                    if (state.mode == Mode::Robot && state.jumping && holdJump &&
                        !state.robotBoostInvalidated && state.robotBoostCharge < 1.5f) {
                        state.robotBoostCharge += scaledDt * 0.1f;
                        state.velocityY += modifiedGravity * scaledDt * gravityFlip * gravityScale;
                    }
                    state.velocityY -= gravityScale * modifiedGravity * scaledDt * gravityFlip;
                    if (!state.upsideDown) state.velocityY = std::max(state.velocityY, -15.f);
                    else state.velocityY = std::min(state.velocityY, 15.f);
                }
                break;
            }
            case Mode::Ship: {
                const bool falling = state.upsideDown ? state.velocityY > 0.f : state.velocityY < 0.f;
                float shipMod = holdJump && !state.velocityUncapped ? -1.f :
                                (state.velocityUncapped ? (falling ? 0.8f : -1.f) : 0.8f);
                if (!holdJump && !falling) shipMod = 1.2f;
                const float boostMod = holdJump && falling ? 0.5f : 0.4f;
                const float gravity = shipMod >= 0.f ? modifiedGravity : 0.958199f;
                state.velocityY -= gravity * scaledDt * gravityFlip * shipMod * boostMod / scaleMod;
                const float cap = 8.f / scaleMod;
                state.velocityY = std::clamp(state.velocityY, -cap, cap);
                break;
            }
            case Mode::Ufo: {
                if (holdJump && state.jumpUnused) {
                    state.velocityY = 7.f * gravityFlip * scaleMod;
                    state.jumpUnused = false;
                }
                const bool falling = state.upsideDown ? state.velocityY > 0.f : state.velocityY < 0.f;
                const float birdMod = falling ? 0.8f : 1.2f;
                state.velocityY -= modifiedGravity * scaledDt * gravityFlip * birdMod * 0.5f / scaleMod;
                const float cap = 8.f / scaleMod;
                state.velocityY = std::clamp(state.velocityY, -cap, cap);
                break;
            }
            case Mode::Wave: {
                const float direction = holdJump ? 1.f : -1.f;
                state.velocityY = horizontalSpeed * gravityFlip * direction;
                break;
            }
            case Mode::Swing: {
                if (holdJump && state.jumpUnused) {
                    state.upsideDown = !state.upsideDown;
                    state.velocityY *= 0.8f;
                    state.jumpUnused = false;
                }
                const float swingScale = state.scale == 1.f ? 0.4f : 0.6f;
                state.velocityY -= swingScale * modifiedGravity * scaledDt * flip(state);
                state.velocityY = std::clamp(state.velocityY, -8.f, 8.f);
                break;
            }
            case Mode::Spider:
            case Mode::Unsupported:
                out.reason = StopReason::UnsupportedMode;
                out.reliable = false;
                return out;
        }

        const float dx = horizontalSpeed * dt * (state.goingLeft ? -1.f : 1.f);
        float dy = state.mode == Mode::Wave
            ? std::fabs(dx) * flip(state) * (holdJump ? 1.f : -1.f)
            : state.velocityY * scaledDt;
        if (state.mode == Mode::Wave && state.scale != 1.f) dy *= 2.f;
        state.position.x += dx;
        state.position.y += dy;
        state.holdingJump = holdJump;

        const Bounds player = playerBounds(state);
        bool stopped = false;
        for (const auto& obstacle : obstacles) {
            if (!overlaps(player, obstacle.bounds)) continue;
            out.reason = obstacle.kind == ObjectKind::Hazard ? StopReason::Hazard :
                         obstacle.kind == ObjectKind::Solid ? StopReason::SolidContact :
                         StopReason::UnsupportedInteraction;
            out.reliable = obstacle.kind != ObjectKind::Interaction &&
                           obstacle.kind != ObjectKind::Slope;
            stopped = true;
            break;
        }
        out.samples.push_back({state.position, static_cast<float>(i + 1) * dt});
        if (stopped) return out;
    }

    out.reason = StopReason::Horizon;
    return out;
}

struct Branches {
    Result hold;
    Result release;
};

inline Branches predictBoth(const State& state, const std::vector<Obstacle>& obstacles,
                            Request request = {}) {
    return {predict(state, true, obstacles, request),
            predict(state, false, obstacles, request)};
}

} // namespace geodan::core::trajectory
