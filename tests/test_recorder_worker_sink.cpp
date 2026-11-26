// Run: g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. tests/test_recorder_worker_sink.cpp -o /tmp/claude-0/tws
// Drives RecorderWorker with an injected encoder (no ffmpeg needed).
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "recorder_worker.hpp"

namespace rec = geodan::core::recorder;
namespace vid = geodan::core::video;

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

namespace {

constexpr std::uint32_t kWidth = 16;
constexpr std::uint32_t kHeight = 64;
constexpr int kFrames = 30;
constexpr std::uint64_t kFrameNs = 33'333'333ull;

struct Counters {
    int frames = 0;
    bool opened = false;
    bool closed = false;
};

// Records what the worker sends it. failAt < 0 means never fail.
class FakeSink final : public vid::VideoSink {
public:
    FakeSink(Counters* counters, int failAt) : m_counters(counters), m_failAt(failAt) {}
    bool open(const vid::SinkConfig&) override { m_counters->opened = true; return true; }
    bool writeFrame(const std::uint8_t*, std::size_t size) override {
        if (size != kWidth * kHeight * 4u) { m_error = "bad size"; return false; }
        if (m_failAt >= 0 && m_counters->frames == m_failAt) { m_error = "fake write failure"; return false; }
        ++m_counters->frames;
        return true;
    }
    bool close() override { m_counters->closed = true; return true; }
    const std::string& error() const override { return m_error; }
private:
    Counters* m_counters;
    int m_failAt;
    std::string m_error;
};

std::string readAll(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// Runs one session with a fake encoder. Returns the worker's success flag.
bool runSession(const std::filesystem::path& session, Counters& counters, int failAt,
                bool& outManifestComplete, bool& outManifestIncomplete) {
    rec::PixelBufferPool pool(kFrames, kWidth * kHeight * 4u);
    rec::BoundedFrameQueue<rec::QueuedFrame> queue(kFrames);
    for (int i = 0; i < kFrames; ++i) {
        auto pixels = pool.tryAcquire();
        if (!pixels) return false;
        rec::FrameGeometry geometry;
        geometry.sequence = static_cast<std::uint64_t>(i);
        const rec::FrameStamp stamp{
            static_cast<std::uint64_t>(i),
            static_cast<std::uint64_t>(i) * kFrameNs,
            i == 0 ? 0ull : kFrameNs,
            0u};
        if (!queue.tryPush(rec::QueuedFrame{stamp, std::move(*pixels), std::move(geometry)}))
            return false;
    }
    bool success = false;
    {
        rec::RecorderWorker worker(queue);
        worker.setVideoSink(std::make_unique<FakeSink>(&counters, failAt));
        rec::SessionConfig config{kWidth, kHeight, 30u, rec::FrameStorage::Mp4};
        if (!worker.start(session, config, 1'700'000'000'000'000'000ull)) return false;
        (void)worker.stop(0, "test_complete");
        success = worker.succeeded();
    }
    const std::string manifest = readAll(session / "manifest.json");
    outManifestComplete = manifest.find("\"status\": \"complete\"") != std::string::npos;
    outManifestIncomplete = manifest.find("\"status\": \"incomplete\"") != std::string::npos;
    return success;
}

}  // namespace

int main() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() /
                      ("geodan-sink-worker-test-" + std::to_string(stamp));

    // Happy path: every frame reaches the encoder, which is opened and closed.
    {
        Counters counters;
        bool complete = false, incomplete = false;
        const bool ok = runSession(root / "good", counters, -1, complete, incomplete);
        CHECK(ok);
        CHECK(counters.opened);
        CHECK(counters.frames == kFrames);
        CHECK(counters.closed);
        CHECK(complete);
        CHECK(!incomplete);
        CHECK(std::filesystem::exists(root / "good" / "frames.csv"));
        CHECK(!std::filesystem::exists(root / "good" / "frames"));
    }

    // Encoder failure mid-recording: the session must not claim it is complete.
    {
        Counters counters;
        bool complete = false, incomplete = false;
        const bool ok = runSession(root / "bad", counters, 5, complete, incomplete);
        CHECK(!ok);
        CHECK(counters.frames == 5);
        CHECK(!complete);
        CHECK(incomplete);
    }

    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    if (g_fail) { std::printf("%d failure(s)\n", g_fail); return EXIT_FAILURE; }
    std::puts("recorder worker with injected encoder: success and failure paths passed");
    return EXIT_SUCCESS;
}
