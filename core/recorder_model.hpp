// GeoDan Level Recorder - portable capture scheduling and backpressure model.
// This header deliberately contains no Geode/Cocos/OpenGL dependencies so its
// timing and queue contracts can be tested without launching Geometry Dash.
#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace geodan::core::recorder {

inline constexpr std::uint32_t kDefaultWidth = 360;
inline constexpr std::uint32_t kDefaultHeight = 1080;
inline constexpr std::uint32_t kDefaultFramesPerSecond = 60;
inline constexpr std::size_t kDefaultQueueCapacity = 3;
inline constexpr std::size_t kBytesPerPixel = 4; // RGBA8

struct Config {
    std::uint32_t width = kDefaultWidth;
    std::uint32_t height = kDefaultHeight;
    std::uint32_t framesPerSecond = kDefaultFramesPerSecond;
    std::size_t queueCapacity = kDefaultQueueCapacity;

    constexpr bool valid() const {
        return width > 0 && height > 0 && framesPerSecond > 0 &&
               queueCapacity > 0 &&
               static_cast<std::uint64_t>(width) * height <=
                   std::numeric_limits<std::size_t>::max() / kBytesPerPixel;
    }

    constexpr std::size_t bytesPerFrame() const {
        if (!valid()) return 0;
        return static_cast<std::size_t>(width) * height * kBytesPerPixel;
    }

    constexpr std::uint64_t rawBytesPerSecond() const {
        return static_cast<std::uint64_t>(bytesPerFrame()) * framesPerSecond;
    }

    constexpr std::size_t queuedBytesUpperBound() const {
        if (!valid() || queueCapacity >
                std::numeric_limits<std::size_t>::max() / bytesPerFrame())
            return 0;
        return queueCapacity * bytesPerFrame();
    }
};

// One stamp is produced for each real captured render. We never synthesize
// duplicate frames to hide a missed render; timestampNs preserves real timing.
struct FrameStamp {
    std::uint64_t sequence = 0;          // contiguous capture sequence
    std::uint64_t timestampNs = 0;       // monotonic time since recording start
    std::uint64_t deltaNs = 0;           // time since previous captured frame
    std::uint32_t missedIntervals = 0;   // expected 60 Hz slots with no capture
};

class FrameTimeline {
public:
    explicit FrameTimeline(std::uint32_t framesPerSecond = kDefaultFramesPerSecond)
      : m_fps(framesPerSecond) {}

    bool valid() const { return m_fps > 0 && m_fps <= 1'000'000'000u; }

    std::optional<FrameStamp> capture(std::uint64_t timestampNs) {
        if (!valid()) return std::nullopt;
        if (m_started && (timestampNs <= m_lastTimestampNs ||
                          timestampNs < m_nextDeadlineNs))
            return std::nullopt;

        FrameStamp stamp;
        stamp.sequence = m_nextSequence++;
        stamp.timestampNs = timestampNs;
        if (m_started) {
            stamp.deltaNs = timestampNs - m_lastTimestampNs;
            const auto expected = 1'000'000'000ull / m_fps;
            if (expected > 0 && timestampNs > m_nextDeadlineNs) {
                const auto missed = (timestampNs - m_nextDeadlineNs) / expected;
                stamp.missedIntervals = static_cast<std::uint32_t>(
                    std::min<std::uint64_t>(missed,
                        std::numeric_limits<std::uint32_t>::max()));
                m_totalMissed += stamp.missedIntervals;
            }
        }
        m_started = true;
        m_lastTimestampNs = timestampNs;
        const auto expected = 1'000'000'000ull / m_fps;
        m_nextDeadlineNs = timestampNs > std::numeric_limits<std::uint64_t>::max() - expected
            ? std::numeric_limits<std::uint64_t>::max()
            : timestampNs + expected;
        ++m_captured;
        return stamp;
    }

    void reset() {
        m_started = false;
        m_lastTimestampNs = 0;
        m_nextDeadlineNs = 0;
        m_nextSequence = 0;
        m_captured = 0;
        m_totalMissed = 0;
    }

    std::uint64_t captured() const { return m_captured; }
    std::uint64_t missedIntervals() const { return m_totalMissed; }
    std::uint32_t framesPerSecond() const { return m_fps; }

private:
    std::uint32_t m_fps = 0;
    bool m_started = false;
    std::uint64_t m_lastTimestampNs = 0;
    std::uint64_t m_nextDeadlineNs = 0;
    std::uint64_t m_nextSequence = 0;
    std::uint64_t m_captured = 0;
    std::uint64_t m_totalMissed = 0;
};

// Bounded non-blocking producer queue. The render thread uses tryPush only:
// it must never wait for disk I/O or image compression. A full queue rejects
// the incoming frame and increments a visible drop counter.
template <typename T>
class BoundedFrameQueue {
public:
    explicit BoundedFrameQueue(std::size_t capacity) : m_capacity(capacity) {}

    BoundedFrameQueue(const BoundedFrameQueue&) = delete;
    BoundedFrameQueue& operator=(const BoundedFrameQueue&) = delete;

    bool tryPush(T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_closed || m_capacity == 0 || m_queue.size() >= m_capacity) {
            ++m_dropped;
            return false;
        }
        m_queue.emplace_back(std::move(value));
        ++m_accepted;
        m_ready.notify_one();
        return true;
    }

    std::optional<T> tryPop() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return std::nullopt;
        T value = std::move(m_queue.front());
        m_queue.pop_front();
        ++m_popped;
        return value;
    }

    // Intended only for the encoder/writer thread, never the game thread.
    std::optional<T> waitPop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_ready.wait(lock, [this] { return m_closed || !m_queue.empty(); });
        if (m_queue.empty()) return std::nullopt;
        T value = std::move(m_queue.front());
        m_queue.pop_front();
        ++m_popped;
        return value;
    }

    void close() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_closed = true;
        m_ready.notify_all();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    std::uint64_t accepted() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_accepted;
    }

    std::uint64_t dropped() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_dropped;
    }

    std::uint64_t popped() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_popped;
    }

    bool closed() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_closed;
    }

private:
    const std::size_t m_capacity;
    mutable std::mutex m_mutex;
    std::condition_variable m_ready;
    std::deque<T> m_queue;
    bool m_closed = false;
    std::uint64_t m_accepted = 0;
    std::uint64_t m_dropped = 0;
    std::uint64_t m_popped = 0;
};

// Fixed-size reusable RGBA storage. Acquire is non-blocking: the render thread
// skips a frame if every slot is in use rather than allocating or waiting.
// A Lease returns its slot automatically after the writer releases it.
class PixelBufferPool {
public:
    class Lease {
    public:
        Lease() = default;
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        Lease(Lease&& other) noexcept;
        Lease& operator=(Lease&& other) noexcept;
        ~Lease();

        explicit operator bool() const { return m_owner != nullptr; }
        std::uint8_t* data();
        const std::uint8_t* data() const;
        std::size_t size() const;
        std::size_t index() const { return m_index; }
        void reset();

    private:
        friend class PixelBufferPool;
        static constexpr std::size_t kInvalidIndex =
            std::numeric_limits<std::size_t>::max();
        Lease(PixelBufferPool* owner, std::size_t index)
          : m_owner(owner), m_index(index) {}
        PixelBufferPool* m_owner = nullptr;
        std::size_t m_index = kInvalidIndex;
    };

    PixelBufferPool(std::size_t bufferCount, std::size_t bytesPerBuffer)
      : m_buffers(bufferCount, std::vector<std::uint8_t>(bytesPerBuffer)) {
        m_free.reserve(bufferCount);
        for (std::size_t i = bufferCount; i > 0; --i) m_free.push_back(i - 1);
    }

    PixelBufferPool(const PixelBufferPool&) = delete;
    PixelBufferPool& operator=(const PixelBufferPool&) = delete;

    std::optional<Lease> tryAcquire() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_free.empty()) return std::nullopt;
        const auto index = m_free.back();
        m_free.pop_back();
        return Lease(this, index);
    }

    std::size_t available() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_free.size();
    }

    std::size_t bufferCount() const { return m_buffers.size(); }
    std::size_t bytesPerBuffer() const {
        return m_buffers.empty() ? 0 : m_buffers.front().size();
    }

private:
    void release(std::size_t index) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_free.push_back(index);
    }

    mutable std::mutex m_mutex;
    std::vector<std::vector<std::uint8_t>> m_buffers;
    std::vector<std::size_t> m_free;
};

// Lease methods are defined after PixelBufferPool is complete.
inline PixelBufferPool::Lease::Lease(Lease&& other) noexcept
  : m_owner(std::exchange(other.m_owner, nullptr)),
    m_index(std::exchange(other.m_index, kInvalidIndex)) {}

inline PixelBufferPool::Lease& PixelBufferPool::Lease::operator=(Lease&& other) noexcept {
    if (this == &other) return *this;
    reset();
    m_owner = std::exchange(other.m_owner, nullptr);
    m_index = std::exchange(other.m_index, kInvalidIndex);
    return *this;
}

inline PixelBufferPool::Lease::~Lease() { reset(); }

inline std::uint8_t* PixelBufferPool::Lease::data() {
    return *this ? m_owner->m_buffers[m_index].data() : nullptr;
}

inline const std::uint8_t* PixelBufferPool::Lease::data() const {
    return *this ? m_owner->m_buffers[m_index].data() : nullptr;
}

inline std::size_t PixelBufferPool::Lease::size() const {
    return *this ? m_owner->m_buffers[m_index].size() : 0;
}

inline void PixelBufferPool::Lease::reset() {
    if (!m_owner) return;
    auto* owner = std::exchange(m_owner, nullptr);
    const auto index = std::exchange(m_index, kInvalidIndex);
    owner->release(index);
}

} // namespace geodan::core::recorder
