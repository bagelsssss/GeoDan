// GeoDan Level Recorder - live capture bridge implementation.
// Per frame: pace with FrameTimeline, render the portrait target, copy one RGBA
// frame into a pooled buffer, and hand it to the bounded queue without waiting.
#include "recorder_live.hpp"

#include <Geode/Geode.hpp>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>

using namespace geode::prelude;

#include "core/recorder_geometry.hpp"
#include "core/recorder_input_log.hpp"
#include "core/recorder_unsupported_log.hpp"
#include "core/recorder_model.hpp"
#include "core/recorder_session.hpp"
#include "core/recorder_worker.hpp"
#include "ffmpeg_api_sink.hpp"
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
    rec::InputLog inputs;         // button presses and releases, game thread only
    rec::UnsupportedCounts unsupportedCounts; // reasons frames were unsupported, game thread only
};

namespace {
LiveRecorder* g_activeRecorder = nullptr;
} // namespace

LiveRecorder* activeRecorder() { return g_activeRecorder; }

LiveRecorder::LiveRecorder() = default;

LiveRecorder::~LiveRecorder() {
    if (m_session) stop("layer_destroyed");
}

void LiveRecorder::update(PlayLayer* layer, bool enabled, const Scene* scene) {
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
    if (!m_session) return;
    if (!scene) {
        m_session->lastDrop = "no_scene";
        return;
    }
    captureFrame(layer, *scene);
}

bool LiveRecorder::start(PlayLayer* layer) {
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
    config.storage = rec::FrameStorage::Mp4;  // one video.mp4, not a PNG per frame
    if (layer && layer->m_level) config.levelName = std::string(layer->m_level->m_levelName.c_str());
    // In-process encoder from the FFmpeg API mod; no ffmpeg.exe on the PATH is needed.
    session->worker.setVideoSink(std::make_unique<encoder::FfmpegApiSink>());

    if (!session->worker.start(session->directory, config, unixStart)) {
        log::error("GeoDan recorder: session did not start at {}: {}",
                   session->directory.string(), session->worker.error());
        m_startFailed = true;
        return false; // session destructor joins the worker thread
    }

    session->startSteadyNs = steadyNowNs();
    log::info("GeoDan recorder: recording to {}", session->directory.string());
    m_session = std::move(session);
    g_activeRecorder = this;
    return true;
}

void LiveRecorder::recordInput(bool down, int button, bool player1) {
    if (!m_session) return;
    auto& s = *m_session;
    rec::InputEvent event;
    event.timeNs = steadyNowNs() - s.startSteadyNs;
    event.down = down;
    event.button = button;
    event.player1 = player1;
    // A full log drops the event and counts it; capture carries on.
    s.inputs.push(event);
}

void LiveRecorder::captureFrame(PlayLayer* layer, const Scene& scene) {
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

    if (scene.root == nullptr || scene.worldToPointScale <= 0.f) {
        s.lastDrop = "no_scene";
        return;
    }
    // The same hitbox-only subtree, zoom and focus as the on-screen preview.
    // Synchronous GPU readback straight from the render target: exactly
    // 360x1080 RGBA, rows top-down.
    scene.root->setVisible(true);
    const bool rendered = s.target.renderAndRead(scene.root, scene.focus,
                                                 scene.worldToPointScale,
                                                 lease.data(), lease.size());
    scene.root->setVisible(false);
    if (!rendered) {
        s.lastDrop = "render_fail";
        return;
    }

    rec::CaptureTransform transform;
    transform.width = rec::kDefaultWidth;
    transform.height = rec::kDefaultHeight;
    transform.contentScale = CC_CONTENT_SCALE_FACTOR();
    transform.worldToPointScale = scene.worldToPointScale;
    transform.focusWorld.x = static_cast<double>(scene.focus.x);
    transform.focusWorld.y = static_cast<double>(scene.focus.y);

    const bool playerDead = !layer->m_player1 || layer->m_player1->m_isDead;
    const auto phase = playerDead ? rec::CapturePhase::Unknown
                                  : rec::CapturePhase::ActiveGameplay;
    auto geometry = capture::snapshotGeometry(layer, stamp->sequence, phase, transform);
    rec::addFrameNotes(s.unsupportedCounts, geometry.unsupportedNotes);

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
        + " qfull " + std::to_string(s.queueFull) + " in " + std::to_string(s.inputs.events().size())
        + " last " + s.lastDrop;
}

void LiveRecorder::stop(const std::string& reason) {
    m_startFailed = false; // the next attempt may start again
    if (g_activeRecorder == this) g_activeRecorder = nullptr;
    if (!m_session) return;
    auto& s = *m_session;

    const bool ok = s.worker.stop(s.timeline.missedIntervals(), reason);

    // The input log is small text, so it is written here on the game thread
    // after the video worker has finished. Written even when the session was
    // incomplete, so the attempt's inputs are kept.
    {
        std::ofstream out(s.directory / "inputs.csv", std::ios::binary | std::ios::trunc);
        out << rec::inputCsv(s.inputs.events());
        out.close();
        if (out.fail()) log::error("GeoDan recorder: could not write inputs.csv in {}", s.directory.string());
        std::ofstream reasons(s.directory / "unsupported_reasons.csv", std::ios::binary | std::ios::trunc);
        reasons << rec::unsupportedCsv(s.unsupportedCounts);
        reasons.close();
        if (s.inputs.dropped() > 0)
            log::warn("GeoDan recorder: {} input events dropped (log full)", s.inputs.dropped());
    }
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
