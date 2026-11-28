// Run: g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. tests/test_recorder_video_worker.cpp -o /tmp/claude-0/tvw
// Needs ffmpeg and ffprobe on PATH (or GEODAN_FFMPEG / GEODAN_FFPROBE). Skips if ffmpeg is missing.
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "recorder_worker.hpp"

namespace rec = geodan::core::recorder;

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

namespace {

constexpr std::uint32_t kWidth = 16;
constexpr std::uint32_t kHeight = 64;
constexpr std::uint32_t kFps = 30;
constexpr int kFrames = 30;
constexpr std::uint64_t kFrameNs = 33'333'333ull;

std::string readAll(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

bool commandRuns(const std::string& exe) {
#if defined(_WIN32)
    const std::string sink = " > NUL 2>&1";
#else
    const std::string sink = " > /dev/null 2>&1";
#endif
    return std::system(("\"" + exe + "\" -version" + sink).c_str()) == 0;
}

// Frame count ffprobe reads from the file, or -1 when it cannot tell.
long long probeFrameCount(const std::string& ffprobe, const std::filesystem::path& file) {
    const std::string cmd = "\"" + ffprobe + "\" -v error -count_frames -select_streams v:0 "
                            "-show_entries stream=nb_read_frames -of csv=p=0 \"" +
                            file.string() + "\"";
    std::FILE* pipe = GEODAN_POPEN(cmd.c_str(), "r");
    if (!pipe) return -1;
    char buffer[64] = {};
    const bool got = std::fgets(buffer, sizeof buffer, pipe) != nullptr;
    GEODAN_PCLOSE(pipe);
    return got ? std::atoll(buffer) : -1;
}

}  // namespace

int main() {
    const char* envFfmpeg = std::getenv("GEODAN_FFMPEG");
    const char* envFfprobe = std::getenv("GEODAN_FFPROBE");
    const std::string ffmpeg = envFfmpeg ? envFfmpeg : "ffmpeg";
    const std::string ffprobe = envFfprobe ? envFfprobe : "ffprobe";
    if (!commandRuns(ffmpeg)) {
        std::printf("skip: ffmpeg not available\n");
        return EXIT_SUCCESS;
    }

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() /
                      ("geodan-video-worker-test-" + std::to_string(stamp));
    const auto session = root / "session";

    // Every frame is leased before the worker starts, so the queue holds them all.
    rec::PixelBufferPool pool(kFrames, kWidth * kHeight * 4u);
    rec::BoundedFrameQueue<rec::QueuedFrame> queue(kFrames);
    for (int i = 0; i < kFrames; ++i) {
        auto pixels = pool.tryAcquire();
        if (!pixels) return EXIT_FAILURE;
        auto* data = pixels->data();
        const std::size_t bytes = pixels->size();
        for (std::size_t p = 0; p < bytes; p += 4) {
            data[p] = static_cast<std::uint8_t>(i * 8);
            data[p + 1] = 0;
            data[p + 2] = 255;
            data[p + 3] = 255;
        }
        rec::FrameGeometry geometry;   // Unsupported: valid without a player
        geometry.sequence = static_cast<std::uint64_t>(i);
        const rec::FrameStamp frameStamp{
            static_cast<std::uint64_t>(i),
            static_cast<std::uint64_t>(i) * kFrameNs,
            i == 0 ? 0ull : kFrameNs,
            0u};
        CHECK(queue.tryPush(rec::QueuedFrame{frameStamp, std::move(*pixels), std::move(geometry)}));
    }

    {
        rec::RecorderWorker worker(queue);
        worker.setFfmpegPath(ffmpeg);
        rec::SessionConfig config{kWidth, kHeight, kFps, rec::FrameStorage::Mp4};
        if (!worker.start(session, config, 1'700'000'000'000'000'000ull)) {
            std::printf("FAIL start: %s\n", worker.error().c_str());
            return EXIT_FAILURE;
        }
        // The session started with dual mode off; the recorder saw it turn on later.
        if (!worker.stop(0, "test_complete", true)) {
            std::printf("FAIL stop: %s\n", worker.error().c_str());
            return EXIT_FAILURE;
        }
        CHECK(worker.succeeded());
        CHECK(worker.framesWritten() == static_cast<std::uint64_t>(kFrames));
    }

    const auto video = session / "video.mp4";
    CHECK(std::filesystem::exists(video));
    if (std::filesystem::exists(video)) CHECK(std::filesystem::file_size(video) > 200);
    CHECK(probeFrameCount(ffprobe, video) == kFrames);
    CHECK(!std::filesystem::exists(session / "frames"));   // no per-frame PNGs

    const std::string manifest = readAll(session / "manifest.json");
    CHECK(manifest.find("\"schema_version\": 2") != std::string::npos);
    CHECK(manifest.find("\"frame_storage\": \"mp4\"") != std::string::npos);
    CHECK(manifest.find("\"video_file\": \"video.mp4\"") != std::string::npos);
    CHECK(manifest.find("\"input_file\": \"inputs.csv\"") != std::string::npos);
    CHECK(manifest.find("\"level_name\": null") != std::string::npos); // none set in this test
    CHECK(manifest.find("\"dual_mode\": true") != std::string::npos); // set at stop, not at start
    CHECK(manifest.find("\"frames_directory\": null") != std::string::npos);
    CHECK(manifest.find("\"status\": \"complete\"") != std::string::npos);
    CHECK(manifest.find("\"frames_written\": 30,") != std::string::npos);

    // One header plus one row per frame.
    const std::string csv = readAll(session / "frames.csv");
    std::size_t lines = 0;
    for (char c : csv) if (c == '\n') ++lines;
    CHECK(lines == static_cast<std::size_t>(kFrames) + 1);

    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    if (g_fail) { std::printf("%d failure(s)\n", g_fail); return EXIT_FAILURE; }
    std::puts("recorder video worker: MP4 encode, metadata and finalization checks passed");
    return EXIT_SUCCESS;
}
