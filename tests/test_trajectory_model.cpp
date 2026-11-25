#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "trajectory_model.hpp"

namespace tr = geodan::core::trajectory;
static int failures = 0;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); ++failures; } } while (0)

int main() {
    tr::State s;
    s.position = {0.f, 100.f};
    s.velocityY = 0.f;
    s.mode = tr::Mode::Wave;
    s.playerSpeed = 0.9f;
    s.speedMultiplier = 5.77f;

    auto branches = tr::predictBoth(s, {});
    CHECK(branches.hold.samples.size() == tr::kDefaultSteps + 1);
    CHECK(branches.release.samples.size() == tr::kDefaultSteps + 1);
    CHECK(branches.hold.reason == tr::StopReason::Horizon);
    CHECK(branches.hold.samples.back().position.y > s.position.y);
    CHECK(branches.release.samples.back().position.y < s.position.y);

    tr::Obstacle hazard{{20.f, 90.f, 30.f, 110.f}, tr::ObjectKind::Hazard};
    tr::Request shortRun{20, tr::kStepSeconds};
    auto hit = tr::predict(s, true, {hazard}, shortRun);
    CHECK(hit.reason == tr::StopReason::Hazard);
    CHECK(hit.reliable);
    CHECK(hit.samples.size() < shortRun.maxSteps + 1);

    tr::Obstacle trigger{{20.f, 90.f, 30.f, 110.f}, tr::ObjectKind::Interaction};
    auto uncertain = tr::predict(s, true, {trigger}, shortRun);
    CHECK(uncertain.reason == tr::StopReason::UnsupportedInteraction);
    CHECK(!uncertain.reliable);

    tr::State unsupported = s;
    unsupported.isPlatformer = true;
    auto noPlatformer = tr::predict(unsupported, true, {}, shortRun);
    CHECK(noPlatformer.reason == tr::StopReason::UnsupportedMode);
    CHECK(!noPlatformer.reliable);
    CHECK(noPlatformer.samples.size() == 1);

    tr::State invalid = s;
    invalid.scale = 0.f;
    auto bad = tr::predict(invalid, true, {}, shortRun);
    CHECK(bad.reason == tr::StopReason::InvalidState);
    CHECK(!bad.reliable);

    tr::State cube = s;
    cube.mode = tr::Mode::Cube;
    cube.position = {0.f, 100.f};
    cube.grounded = true;
    cube.canJump = true;
    cube.holdingJump = false;
    cube.jumpVelocity = 11.f;
    auto jump = tr::predict(cube, true, {}, {8, tr::kStepSeconds});
    auto noJump = tr::predict(cube, false, {}, {8, tr::kStepSeconds});
    CHECK(jump.samples.back().position.y > noJump.samples.back().position.y);

    if (failures == 0) std::puts("trajectory model: all checks passed");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
