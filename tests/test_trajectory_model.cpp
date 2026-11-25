// Unit checks for core/trajectory_model.hpp.
// Build: g++ -std=c++20 -Wall -Wextra -Werror -Icore tests/test_trajectory_model.cpp -o t && ./t
#include <cstdio>
#include <cstdlib>
#include <string>

#include "trajectory_model.hpp"

namespace tj = geodan::core::trajectory;

static int failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                          \
        }                                                                        \
    } while (0)

int main() {
    CHECK(tj::kStepSeconds == 1.f / 240.f);

    CHECK(tj::clampSteps(0) == 1);
    CHECK(tj::clampSteps(-3) == 1);
    CHECK(tj::clampSteps(480) == 480);
    CHECK(tj::clampSteps(100000) == 960);

    CHECK(std::string(tj::stopReasonName(tj::StopReason::HorizonReached)) == "horizon_reached");
    CHECK(std::string(tj::stopReasonName(tj::StopReason::HazardDeath)) == "hazard_death");
    CHECK(std::string(tj::stopReasonName(tj::StopReason::UnsupportedTrigger)) == "unsupported_trigger");

    tj::Branch b;
    b.stop = tj::StopReason::HorizonReached;
    CHECK(!b.stoppedEarly());
    b.stop = tj::StopReason::ModeSwitch;
    CHECK(b.stoppedEarly());

    b.completedSteps = 240;
    CHECK(b.horizonSeconds() > 0.99f && b.horizonSeconds() < 1.01f);

    CHECK(tj::needsRefresh(true, false, 1));
    CHECK(tj::needsRefresh(false, true, 1));
    CHECK(tj::needsRefresh(false, false, 8));
    CHECK(!tj::needsRefresh(false, false, 9));

    tj::InputState a, c;
    CHECK(a == c);
    c.holding = true;
    CHECK(!(a == c));

    tj::Branch plain;
    plain.requestedSteps = 480;
    plain.completedSteps = 480;
    plain.stop = tj::StopReason::HorizonReached;
    const std::string line = tj::describe("hold", plain);
    CHECK(line.find("steps=480/480") != std::string::npos);
    CHECK(line.find("stop=horizon_reached") != std::string::npos);
    CHECK(line.find("guess_from") == std::string::npos);

    tj::Branch guessed = plain;
    guessed.guessFrom = 120;
    guessed.guessObjectID = 1934;
    guessed.triggersPassed = 2;
    const std::string line2 = tj::describe("release", guessed);
    CHECK(line2.find("guess_from=120") != std::string::npos);
    CHECK(line2.find("trigger=1934") != std::string::npos);
    CHECK(line2.find("passed=2") != std::string::npos);

    if (failures == 0) std::puts("trajectory model: all checks passed");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
