// GeoDan Level Recorder - live capture bridge implementation.
// Per frame: pace with FrameTimeline, render the portrait target, copy one RGBA
// frame into a pooled buffer, and hand it to the bounded queue without waiting.
#include "recorder_live.hpp"

#include <Geode/Geode.hpp>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <memory>

using namespace geode::prelude;

#include "core/recorder_geometry.hpp"
#include "core/recorder_model.hpp"
#include "core/recorder_session.hpp"
#include "core/recorder_worker.hpp"
#include "portrait_capture_target.hpp"
#include "recorder_geometry.hpp"
namespace rec = geodan::core::recorder;
namespace capture = geodan::tools::capture;

namespace geodan::tools::recorder_live {

namespace {

// One buffer being rendered, one being read back, on top of the queue slots.
constexpr std::size_t kBufferSlack = 2;

std::uint64_t steadyNowNs() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

} // namespace

struct LiveRecorder::Session {
    // Member order matters: the pool must outlive the queue (queued leases
    // return their slot to it), and the queue must outlive the worker.
    Session(std::size_t bufferCount, std::size_t bytesPerBuffer, std::size_t queueCapacity)
        : pool(bufferCount, bytesPerBuffer), queue(queueCapacity), worker(queue) {}

    rec::FrameTimeline timeline{rec::kDefaultFramesPerSecond};
    rec::PixelBufferPool pool;
    rec::BoundedFrameQueue<rec::QueuedFrame> queue;
    rec::RecorderWorker worker;
    capture::PortraitCaptureTarget target;
    std::uint64_t startSteadyNs = 0;
    std::uint64_t noBufferDrops = 0;
    std::uint64_t queued = 0;     // frames handed to the writer queue
    std::uint64_t queueFull = 0;  // frames the full queue refused
    std::string lastDrop = "none";
    std::filesystem::path directory;
};

LiveRecorder::LiveRecorder() = default;

LiveRecorder::~LiveRecorder() {
    if (m_session) stop("layer_destroyed");
}

void LiveRecorder::update(PlayLayer* layer, bool enabled) {
    if (!enabled) {
        stop("setting_off");
        return;
    }
    if (!layer) return;
    if (!m_session && !m_startFailed && layer->m_player1) {
        if (start(layer))
            Notification::create("GeoDan: recording started", NotificationIcon::Success)->show();
        else
            Notification::create("GeoDan: recording could not start", NotificationIcon::Error)->show();
    }
    if (m_session) captureFrame(layer);
}

bool LiveRecorder::start(PlayLayer* layer) {
    (void)layer;
    const auto width = rec::kDefaultWidth;
    const auto height = rec::kDefaultHeight;
    const auto bytesPerFrame = static_cast<std::size_t>(width) * height * rec::kBytesPerPixel;

    auto session = std::make_unique<Session>(
        rec::kDefaultQueueCapacity + kBufferSlack, bytesPerFrame, rec::kDefaultQueueCapacity);

    if (!session->target.initialize(static_cast<int>(width), static_cast<int>(height))) {
        log::error("GeoDan recorder: could not create an exact {}x{} render target", width, height);
        m_startFailed = true;
        return false;
    }

    const auto unixStart = rec::SessionWriter::nowUnixNs();
    session->directory = Mod::get()->getSaveDir() / "recordings"
        / ("session_" + std::to_string(unixStart));

    rec::SessionConfig config;
    config.width = width;
    config.height = height;
    config.targetFramesPerSecond = rec::kDefaultFramesPerSecond;

    if (!session->worker.start(session->directory, config, unixStart)) {
        log::error("GeoDan recorder: session did not start at {}: {}",
                   session->directory.string(), session->worker.error());
        m_startFailed = true;
        return false; // session destructor joins the worker thread
    }

    session->startSteadyNs = steadyNowNs();
    log::info("GeoDan recorder: recording to {}", session->directory.string());
    m_session = std::move(session);
    return true;
}

void LiveRecorder::captureFrame(PlayLayer* layer) {
    auto& s = *m_session;

    // The timeline paces capture to the 60 Hz cadence. A frame that is not due
    // yet returns nullopt and costs nothing.
    const auto stamp = s.timeline.capture(steadyNowNs() - s.startSteadyNs);
    if (!stamp) return;

    // The sequence number is already consumed. A dropped frame leaves a gap in
    // frames.csv, which the image parser accepts.
    auto leaseOpt = s.pool.tryAcquire();
    if (!leaseOpt) {
        ++s.noBufferDrops;
        s.lastDrop = "no_buffer";
        return;
    }
    rec::PixelBufferPool::Lease lease = std::move(*leaseOpt);

    const auto focus = capture::PortraitCaptureTarget::worldPositionOf(layer->m_player1);
    const auto win = CCDirector::sharedDirector()->getWinSize();
    const int pointHeight = s.target.pointHeight();
    if (!focus || win.height <= 0.f || pointHeight <= 0) { s.lastDrop = "no_focus"; return; }
    const float worldToPointScale = static_cast<float>(pointHeight) / win.height;
    // Synchronous GPU readback straight from the render target: exactly
    // 360x1080 RGBA, rows top-down. Accepted by the user; measure at 60 FPS.
    if (!s.target.renderAndRead(layer->m_objectLayer, *focus, worldToPointScale,
                                lease.data(), lease.size())) {
        s.lastDrop = "render_fail";
        return;
    }

    rec::CaptureTransform transform;
    transform.width = rec::kDefaultWidth;
    transform.height = rec::kDefaultHeight;
    transform.contentScale = CC_CONTENT_SCALE_FACTOR();
    transform.worldToPointScale = worldToPointScale;
    transform.focusWorld.x = static_cast<double>(focus->x);
    transform.focusWorld.y = static_cast<double>(focus->y);

    const bool playerDead = !layer->m_player1 || layer->m_player1->m_isDead;
    const auto phase = playerDead ? rec::CapturePhase::Unknown
                                  : rec::CapturePhase::ActiveGameplay;
    auto geometry = capture::snapshotGeometry(layer, stamp->sequence, phase, transform);

    rec::QueuedFrame frame{*stamp, std::move(lease), std::move(geometry)};
    // Never waits; a full queue counts a drop.
    if (s.queue.tryPush(std::move(frame))) {
        ++s.queued;
    } else {
        ++s.queueFull;
        s.lastDrop = "queue_full";
    }
}

std::string LiveRecorder::status() const {
    if (!m_session) return m_startFailed ? "REC failed to start" : "REC waiting";
    const auto& s = *m_session;
    return "REC queued " + std::to_string(s.queued) + " nobuf " + std::to_string(s.noBufferDrops)
        + " qfull " + std::to_string(s.queueFull) + " last " + s.lastDrop;
}

void LiveRecorder::stop(const std::string& reason) {
    m_startFailed = false; // the next attempt may start again
    if (!m_session) return;
    auto& s = *m_session;

    const bool ok = s.worker.stop(s.timeline.missedIntervals(), reason);
    if (ok) {
        log::info("GeoDan recorder: saved {} frames to {} ({}; no-buffer drops {}, queue drops {})",
                  s.worker.framesWritten(), s.directory.string(), reason,
                  s.noBufferDrops, s.queue.dropped());
    } else {
        log::error("GeoDan recorder: session ended incomplete ({}): {}",
                   reason, s.worker.error());
    }
    m_session.reset();
}

} // namespace geodan::tools::recorder_live
