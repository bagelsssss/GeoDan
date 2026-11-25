// Unit checks for the trajectory prediction data model.
// Build: g++ -std=c++20 -Wall -Wextra -Werror -Icore tests/test_trajectory_model.cpp -o /tmp/test-trajectory && /tmp/test-trajectory
#include <cstdio>
#include <cstdlib>

#include "trajectory_model.hpp"

namespace tr = geodan::core::trajectory;

static int failures = 0;
#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (false)

int main() {
    tr::Prediction prediction;
    const tr::Sample first{{10.f, 20.f}, 45.f};

    CHECK(prediction.path(tr::Branch::Hold).append(first, 3));
    CHECK(prediction.path(tr::Branch::Release).samples.empty());
    CHECK(prediction.path(tr::Branch::Hold).samples.size() == 1);

    // The two branches terminate independently.
    prediction.path(tr::Branch::Hold).stop(tr::StopReason::UnsafeInteraction);
    CHECK(prediction.path(tr::Branch::Hold).stopped());
    CHECK(!prediction.path(tr::Branch::Hold).append(first, 3));
    CHECK(prediction.path(tr::Branch::Release).append(first, 3));

    prediction.path(tr::Branch::Release).finish();
    CHECK(prediction.path(tr::Branch::Release).complete);
    CHECK(!prediction.path(tr::Branch::Release).append(first, 3));

    prediction.reset();
    CHECK(prediction.path(tr::Branch::Hold).samples.empty());
    CHECK(prediction.path(tr::Branch::Hold).stopReason == tr::StopReason::None);
    CHECK(!prediction.path(tr::Branch::Hold).complete);
    CHECK(prediction.path(tr::Branch::Release).samples.empty());

    // A path has a hard bound and reports why it stopped.
    tr::Path bounded;
    CHECK(bounded.append(first, 2));
    CHECK(bounded.append(first, 2));
    CHECK(bounded.stopReason == tr::StopReason::StepLimit);
    CHECK(!bounded.append(first, 2));
    CHECK(bounded.samples.size() == 2);

    // A zero-sized request is normalized to one sample; excessive values clamp.
    tr::Settings settings;
    settings.maxSamples = 0;
    CHECK(settings.boundedMaxSamples() == 1);
    settings.maxSamples = tr::Settings::kHardMaxSamples + 100;
    CHECK(settings.boundedMaxSamples() == tr::Settings::kHardMaxSamples);
    settings.maxSamples = 120;
    CHECK(settings.boundedMaxSamples() == 120);

    if (failures == 0) std::puts("trajectory model: all checks passed");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
