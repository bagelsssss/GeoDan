// Single-owner background encoder/session writer.
// The producer only moves a preallocated pixel lease into the bounded queue.
#pragma once

#include <cstdint>
#include <exception>
#include <optional>
#include <span>
#include <filesystem>
#include <future>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "recorder_model.hpp"
#include "recorder_session.hpp"
#include "recorder_png_encoder.hpp"

namespace geodan::core::recorder {

struct QueuedFrame {
    FrameStamp stamp;
    PixelBufferPool::Lease pixels;
    FrameGeometry geometry;
};

class RecorderWorker final {
public:
    explicit RecorderWorker(BoundedFrameQueue<QueuedFrame>& queue) : m_queue(queue) {}
    RecorderWorker(const RecorderWorker&) = delete;
    RecorderWorker& operator=(const RecorderWorker&) = delete;

    ~RecorderWorker() {
        if (m_started && !m_joined) (void)stop(0, "worker_destroyed");
    }

    // Session creation occurs inside the worker thread so SessionWriter has a
    // single owner for its entire lifetime.
    bool start(const std::filesystem::path& directory, SessionConfig config,
               std::uint64_t startUnixNs) {
        if (m_started || m_queue.closed()) return false;
        m_started = true;
        std::promise<bool> ready;
        auto readyFuture = ready.get_future();
        m_thread = std::thread([this, directory, config, startUnixNs,
                                ready = std::move(ready)]() mutable {
            run(directory, config, startUnixNs, std::move(ready));
        });
        return readyFuture.get();
    }

    // Stop accepting frames before calling this. close() wakes the worker;
    // waitPop drains every accepted frame before the worker finalizes.
    bool stop(std::uint64_t totalMissedCaptureIntervals,
              const std::string& reason = "user_stop") {
        if (!m_started || m_joined) return m_success;
        m_missedAtStop = totalMissedCaptureIntervals;
        m_stopReason = reason;
        m_queue.close();
        if (m_thread.joinable()) m_thread.join();
        m_joined = true;
        return m_success;
    }

    bool joined() const { return m_joined; }
    bool succeeded() const { return m_joined && m_success; }
    const std::string& error() const { return m_error; }
    std::uint64_t framesWritten() const { return m_writer.framesWritten(); }

private:
    void run(const std::filesystem::path& directory, SessionConfig config,
             std::uint64_t startUnixNs, std::promise<bool> ready) noexcept {
        bool readySent = false;
        try {
            if (!m_writer.start(directory, config, startUnixNs)) {
                m_error = m_writer.error().empty() ? "session_start_failed" : m_writer.error();
                ready.set_value(false);
                return;
            }
            ready.set_value(true);
            readySent = true;

            std::vector<std::uint8_t> encoded;
            std::optional<QueuedFrame> frame;
            while ((frame = m_queue.waitPop()).has_value()) {
                if (!m_error.empty()) continue; // release queued leases after first failure
                if (!frame->pixels || frame->pixels.size() !=
                    static_cast<std::size_t>(config.width) * config.height * 4u) {
                    m_error = "invalid_rgba_frame_buffer";
                    continue;
                }
                if (!m_encoder.encode(config.width, config.height,
                        std::span<const std::uint8_t>(frame->pixels.data(), frame->pixels.size()),
                        encoded)) {
                    m_error = "png_encode_failed";
                    continue;
                }
                if (!m_writer.writeFrame(frame->stamp, encoded, frame->geometry)) {
                    m_error = m_writer.error().empty() ? "session_frame_write_failed"
                                                       : m_writer.error();
                }
            }

            m_writer.recordMissedCaptureIntervals(m_missedAtStop);
            m_writer.recordQueueDrops(m_queue.dropped());
            if (m_error.empty()) {
                m_success = m_writer.finalize(SessionWriter::nowUnixNs(), m_stopReason);
                if (!m_success)
                    m_error = m_writer.error().empty() ? "session_finalize_failed" : m_writer.error();
            } else {
                (void)m_writer.abort(m_error, SessionWriter::nowUnixNs());
            }
        } catch (const std::exception& exception) {
            m_error = exception.what();
            if (m_writer.state() == SessionWriter::State::Recording)
                (void)m_writer.abort(m_error, SessionWriter::nowUnixNs());
        } catch (...) {
            m_error = "unknown_recorder_worker_exception";
            if (m_writer.state() == SessionWriter::State::Recording)
                (void)m_writer.abort(m_error, SessionWriter::nowUnixNs());
        }
        if (!readySent) {
            try { ready.set_value(false); } catch (...) {}
        }
    }

    BoundedFrameQueue<QueuedFrame>& m_queue;
    SessionWriter m_writer;
    RgbaPngEncoder m_encoder;
    std::thread m_thread;
    bool m_started = false;
    bool m_joined = false;
    bool m_success = false;
    std::uint64_t m_missedAtStop = 0;
    std::string m_stopReason = "user_stop";
    std::string m_error;
};

} // namespace geodan::core::recorder
