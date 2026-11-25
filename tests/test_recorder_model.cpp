// Run: g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore tests/test_recorder_model.cpp -o /tmp/recorder-test && /tmp/recorder-test
#include <cstdio>
#include <cstdlib>
#include <string>

#include "recorder_model.hpp"

namespace rec = geodan::core::recorder;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

int main() {
    rec::Config config;
    CHECK(config.valid());
    CHECK(config.width == 360);
    CHECK(config.height == 1080);
    CHECK(config.framesPerSecond == 60);
    CHECK(config.bytesPerFrame() == 360u * 1080u * 4u);
    CHECK(config.rawBytesPerSecond() == 360ull * 1080ull * 4ull * 60ull);
    CHECK(config.queuedBytesUpperBound() ==
          360u * 1080u * 4u * rec::kDefaultQueueCapacity);

    auto invalid = config;
    invalid.width = 0;
    CHECK(!invalid.valid());
    CHECK(invalid.bytesPerFrame() == 0);
    invalid = config;
    invalid.framesPerSecond = 0;
    CHECK(!invalid.valid());

    // Real captures retain timestamps; no fake/duplicated frames are created.
    rec::FrameTimeline timeline(60);
    auto first = timeline.capture(0);
    CHECK(first.has_value());
    CHECK(first->sequence == 0);
    CHECK(first->timestampNs == 0);
    CHECK(!timeline.capture(0).has_value()); // timestamps must be strictly increasing
    timeline.reset();
    first = timeline.capture(1'000'000);
    CHECK(first.has_value());
    CHECK(first->sequence == 0);
    CHECK(first->timestampNs == 1'000'000);
    CHECK(first->missedIntervals == 0);

    auto second = timeline.capture(17'666'667);
    CHECK(second.has_value());
    CHECK(second->sequence == 1);
    CHECK(second->deltaNs == 16'666'667);
    CHECK(second->missedIntervals == 0);

    auto late = timeline.capture(67'666'667);
    CHECK(late.has_value());
    CHECK(late->sequence == 2);
    CHECK(late->missedIntervals == 2);
    CHECK(timeline.captured() == 3);
    CHECK(timeline.missedIntervals() == 2);

    CHECK(!timeline.capture(67'666'667).has_value());
    CHECK(!timeline.capture(10).has_value());

    timeline.reset();
    CHECK(timeline.captured() == 0);
    CHECK(timeline.missedIntervals() == 0);
    CHECK(timeline.capture(42).has_value());

    rec::FrameTimeline invalidTimeline(0);
    CHECK(!invalidTimeline.valid());
    CHECK(!invalidTimeline.capture(1).has_value());

    // Queue producer never blocks and never grows beyond its configured bound.
    rec::BoundedFrameQueue<std::string> queue(2);
    CHECK(queue.tryPush("frame-0"));
    CHECK(queue.tryPush("frame-1"));
    CHECK(!queue.tryPush("frame-2"));
    CHECK(queue.size() == 2);
    CHECK(queue.accepted() == 2);
    CHECK(queue.dropped() == 1);

    auto item0 = queue.tryPop();
    CHECK(item0.has_value() && *item0 == "frame-0");
    CHECK(queue.popped() == 1);
    CHECK(queue.tryPush("frame-2"));
    CHECK(queue.size() == 2);

    auto item1 = queue.waitPop();
    CHECK(item1.has_value() && *item1 == "frame-1");
    auto item2 = queue.waitPop();
    CHECK(item2.has_value() && *item2 == "frame-2");

    queue.close();
    CHECK(queue.closed());
    CHECK(!queue.tryPush("after-close"));
    CHECK(queue.dropped() == 2);
    CHECK(!queue.waitPop().has_value());

    rec::BoundedFrameQueue<int> zeroCapacity(0);
    CHECK(!zeroCapacity.tryPush(1));
    CHECK(zeroCapacity.dropped() == 1);

    if (failures == 0) std::puts("recorder model: all checks passed");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
