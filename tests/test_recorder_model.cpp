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


    // When the game renders faster than the requested cadence, intermediate
    // renders are skipped instead of accidentally producing 120 FPS footage.
    rec::FrameTimeline pacedTimeline(60);
    CHECK(pacedTimeline.capture(0).has_value());
    CHECK(!pacedTimeline.capture(8'333'333).has_value());
    auto paced = pacedTimeline.capture(16'666'666);
    CHECK(paced.has_value());
    CHECK(paced->sequence == 1);
    CHECK(paced->missedIntervals == 0);


    // Small sustained lateness accumulates against the anchored clock rather
    // than being hidden by resetting the deadline after every capture.
    rec::FrameTimeline driftTimeline(60);
    CHECK(driftTimeline.capture(0).has_value());
    std::optional<rec::FrameStamp> driftFrame;
    for (std::uint64_t i = 1; i <= 125; ++i)
        driftFrame = driftTimeline.capture(i * 16'800'000ull);
    CHECK(driftFrame.has_value());
    CHECK(driftTimeline.missedIntervals() == 1);

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


    // Pixel buffers are preallocated, leased without blocking, and recycled.
    rec::PixelBufferPool pool(2, 128);
    CHECK(pool.bufferCount() == 2);
    CHECK(pool.bytesPerBuffer() == 128);
    CHECK(pool.available() == 2);
    auto buffer0 = pool.tryAcquire();
    auto buffer1 = pool.tryAcquire();
    CHECK(buffer0.has_value() && static_cast<bool>(*buffer0));
    CHECK(buffer1.has_value() && static_cast<bool>(*buffer1));
    CHECK(pool.available() == 0);
    CHECK(!pool.tryAcquire().has_value());
    CHECK(buffer0->size() == 128);
    buffer0->data()[0] = 0x5a;
    const auto reusedIndex = buffer0->index();
    rec::PixelBufferPool::Lease movedBuffer = std::move(*buffer0);
    CHECK(!static_cast<bool>(*buffer0));
    CHECK(movedBuffer.data()[0] == 0x5a);
    movedBuffer.reset();
    CHECK(pool.available() == 1);
    auto reused = pool.tryAcquire();
    CHECK(reused.has_value());
    CHECK(reused->index() == reusedIndex);
    reused.reset();
    buffer1.reset();
    CHECK(pool.available() == 2);

    if (failures == 0) std::puts("recorder model: all checks passed");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
