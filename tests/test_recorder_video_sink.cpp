#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "recorder_video_sink.hpp"

namespace vid = geodan::core::video;

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

int main() {
    // Command shape, no ffmpeg needed.
    vid::SinkConfig cfg;
    cfg.outputPath = "out/video.mp4";
    cfg.width = 360;
    cfg.height = 1080;
    cfg.fps = 60;
    const std::string cmd = vid::buildCommand(cfg);
    CHECK(cmd.find("-f rawvideo -pix_fmt rgba -s 360x1080 -r 60 -i -") != std::string::npos);
    CHECK(cmd.find("-c:v libx264") != std::string::npos);
    CHECK(cmd.find("\"out/video.mp4\"") != std::string::npos);

    // Invalid configs produce no command.
    vid::SinkConfig bad = cfg;
    bad.width = 0;
    CHECK(vid::buildCommand(bad).empty());
    bad = cfg;
    bad.outputPath = "a\"b.mp4";
    CHECK(vid::buildCommand(bad).empty());
    bad = cfg;
    bad.fps = 0;
    CHECK(vid::buildCommand(bad).empty());

    // A real encode: 30 frames of 16x64 RGBA, then the file must exist and be non-empty.
    vid::SinkConfig small;
    small.ffmpeg = std::getenv("GEODAN_FFMPEG") ? std::getenv("GEODAN_FFMPEG") : "ffmpeg";
    small.outputPath = "recorder_video_sink_test.mp4";
    small.width = 16;
    small.height = 64;
    small.fps = 30;
    vid::FfmpegSink sink;
    if (!sink.open(small)) {
        std::printf("skip: ffmpeg not available (%s)\n", sink.error().c_str());
        if (g_fail) { std::printf("%d failure(s)\n", g_fail); return 1; }
        std::printf("recorder video sink: all checks passed (encode skipped)\n");
        return 0;
    }
    std::vector<std::uint8_t> frame(16u * 64u * 4u);
    for (int i = 0; i < 30; ++i) {
        for (std::size_t p = 0; p < frame.size(); p += 4) {
            frame[p] = static_cast<std::uint8_t>(i * 8);
            frame[p + 1] = 0;
            frame[p + 2] = 255;
            frame[p + 3] = 255;
        }
        CHECK(sink.writeFrame(frame.data(), frame.size()));
    }
    CHECK(!sink.writeFrame(frame.data(), frame.size() - 4));  // wrong size is refused
    CHECK(sink.framesWritten() == 30);
    CHECK(sink.close());

    std::FILE* f = std::fopen("recorder_video_sink_test.mp4", "rb");
    CHECK(f != nullptr);
    if (f) {
        std::fseek(f, 0, SEEK_END);
        CHECK(std::ftell(f) > 200);  // a real MP4 has at least its headers
        std::fclose(f);
    }
    std::remove("recorder_video_sink_test.mp4");

    if (g_fail) { std::printf("%d failure(s)\n", g_fail); return 1; }
    std::printf("recorder video sink: all checks passed\n");
    return 0;
}
