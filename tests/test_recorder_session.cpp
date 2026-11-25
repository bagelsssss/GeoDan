// Run: g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore tests/test_recorder_session.cpp -o /tmp/recorder-session-test && /tmp/recorder-session-test
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include "recorder_session.hpp"

namespace rec = geodan::core::recorder;
static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

static std::string readText(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

int main() {
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() /
                      ("geodan-session-test-" + std::to_string(unique));
    const auto completeDir = root / "complete";
    const auto incompleteDir = root / "incomplete";
    std::filesystem::create_directories(root);
    const std::array<std::uint8_t, 16> pngBytes{
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
        0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44
    };

    {
        rec::SessionWriter writer;
        CHECK(writer.start(completeDir, {360, 1080, 60}, 1'700'000'000'000'000'000ull));
        CHECK(writer.state() == rec::SessionWriter::State::Recording);
        CHECK(std::filesystem::exists(completeDir / "manifest.json"));
        CHECK(std::filesystem::exists(completeDir / "frames.csv"));
        CHECK(std::filesystem::is_directory(completeDir / "frames"));
        CHECK(readText(completeDir / "manifest.json").find("\"status\": \"recording\"") != std::string::npos);
        CHECK(writer.writeFrame({10, 16'666'666, 16'666'666, 0}, pngBytes));
        CHECK(writer.writeFrame({12, 50'000'000, 16'666'667, 1}, pngBytes));
        CHECK(!writer.writeFrame({12, 50'000'000, 16'666'667, 1}, pngBytes));
        CHECK(writer.framesWritten() == 2);
        CHECK(writer.missedIntervals() == 0);
        writer.recordMissedCaptureIntervals(1);
        CHECK(writer.missedIntervals() == 1);
        writer.recordQueueDrops(2);
        CHECK(writer.queueDrops() == 2);
        CHECK(std::filesystem::exists(completeDir / "frames" / "frame_000010.png"));
        CHECK(std::filesystem::exists(completeDir / "frames" / "frame_000012.png"));
        CHECK(!std::filesystem::exists(completeDir / "frames" / "frame_000011.png"));
        CHECK(writer.finalize(1'700'000'001'000'000'000ull, "level_exit"));
        CHECK(writer.state() == rec::SessionWriter::State::Complete);
        CHECK(!writer.writeFrame({3, 50'000'000, 16'666'667, 0}, pngBytes));

        const auto manifest = readText(completeDir / "manifest.json");
        CHECK(manifest.find("\"status\": \"complete\"") != std::string::npos);
        CHECK(manifest.find("\"width\": 360") != std::string::npos);
        CHECK(manifest.find("\"height\": 1080") != std::string::npos);
        CHECK(manifest.find("\"target_fps\": 60") != std::string::npos);
        CHECK(manifest.find("\"frames_written\": 2") != std::string::npos);
        CHECK(manifest.find("\"missed_capture_intervals\": 1") != std::string::npos);
        CHECK(manifest.find("\"writer_queue_drops\": 2") != std::string::npos);
        CHECK(manifest.find("\"first_sequence\": 10") != std::string::npos);
        CHECK(manifest.find("\"last_sequence\": 12") != std::string::npos);
        CHECK(manifest.find("\"stop_reason\": \"level_exit\"") != std::string::npos);
        const auto csv = readText(completeDir / "frames.csv");
        CHECK(csv.find("sequence,timestamp_ns,delta_ns,missed_intervals,write_status") == 0);
        CHECK(csv.find("10,0,0,0,written") != std::string::npos);
        CHECK(csv.find("12,50000000,16666667,1,written") != std::string::npos);
    }
    {
        rec::SessionWriter writer;
        CHECK(writer.start(incompleteDir, {360, 1080, 60}, 1234));
        CHECK(writer.writeFrame({0, 0, 0, 0}, pngBytes));
        CHECK(writer.abort("game_closed", 5678));
        CHECK(writer.state() == rec::SessionWriter::State::Incomplete);
        const auto manifest = readText(incompleteDir / "manifest.json");
        CHECK(manifest.find("\"status\": \"incomplete\"") != std::string::npos);
        CHECK(manifest.find("\"stop_reason\": \"game_closed\"") != std::string::npos);
        CHECK(manifest.find("\"end_unix_ns\": 5678") != std::string::npos);
    }
    {
        rec::SessionWriter writer;
        CHECK(!writer.start(completeDir, {360, 1080, 60}, 1));
    }
    {
        rec::SessionWriter writer;
        CHECK(!writer.start(root / "invalid", {0, 1080, 60}, 1));
    }
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    if (failures == 0) std::puts("recorder session: all checks passed");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
