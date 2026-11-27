// Portable test for the v2 I/I Analyzer. Builds a synthetic session on disk, then
// analyzes it. Build:
// g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. tests/test_input_image_analyzer.cpp
#include "input_image_analyzer.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>

using namespace geodan::core;
namespace fs = std::filesystem;

namespace {

// 200 frames at 60 fps. Frame 0-99 have no player (unsupported); from frame 100 the
// player is 3 px from a hazard (clearance 3 at scale 1), and a press happens at frame 150.
void writeSession(const fs::path& dir, bool complete, bool withInputs) {
    fs::create_directories(dir);
    const std::uint64_t frameNs = 1'000'000'000ull / 60;
    const int frames = 200;
    {
        std::ofstream m(dir / "manifest.json");
        m << "{\n"
          << "  \"schema_version\": 2,\n"
          << "  \"status\": \"" << (complete ? "complete" : "incomplete") << "\",\n"
          << "  \"width\": 360,\n  \"height\": 1080,\n  \"target_fps\": 60,\n"
          << "  \"frames_written\": " << frames << ",\n"
          << "  \"missed_capture_intervals\": 0,\n  \"writer_queue_drops\": 0,\n"
          << "  \"start_unix_ns\": 1000,\n  \"end_unix_ns\": 2000,\n"
          << "  \"first_sequence\": 0,\n  \"last_sequence\": " << frames - 1 << ",\n"
          << "  \"stop_reason\": \"user_quit\",\n"
          << "  \"frames_metadata\": \"frames.csv\",\n"
          << "  \"frame_storage\": \"mp4\",\n"
          << "  \"video_file\": \"video.mp4\",\n"
          << "  \"input_file\": \"inputs.csv\",\n"
          << "  \"level_name\": " << (complete ? "\"Acu\"" : "null") << ",\n"
          << "  \"frames_directory\": null\n}\n";
    }
    {
        std::ofstream f(dir / "frames.csv");
        f << "sequence,timestamp_ns,delta_ns,missed_intervals,write_status\n";
        for (int i = 0; i < frames; ++i)
            f << i << ',' << i * frameNs << ',' << (i ? frameNs : 0) << ",0,written\n";
    }
    {
        std::ofstream g(dir / "hitboxes.csv");
        g << "sequence,phase,geometry_status,entity_type,entity_id,left,top,right,bottom\n";
        for (int i = 0; i < frames; ++i) {
            const bool hasPlayer = i >= 100;
            g << i << ",active_gameplay," << (hasPlayer ? "valid" : "unsupported") << ",frame,,,,,\n";
            if (hasPlayer) {
                g << i << ",active_gameplay,valid,player,1,0,0,10,10\n";
                g << i << ",active_gameplay,valid,hazard,2,13,0,20,10\n";
            }
        }
    }
    {
        std::ofstream v(dir / "video.mp4", std::ios::binary);
        v << "not decoded here";
    }
    if (withInputs) {
        std::ofstream in(dir / "inputs.csv");
        in << "time_ns,down,button,player1\n";
        in << 150 * frameNs + 1 << ",1,1,1\n";   // press during frame 150
        in << 151 * frameNs + 1 << ",0,1,1\n";   // release during frame 151
    } else {
        std::ofstream in(dir / "inputs.csv");
        in << "time_ns,down,button,player1\n";
    }
}

} // namespace

int main() {
    const fs::path root = fs::path(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp")
        / "geodan_ii_analyzer_test";
    std::error_code ec;
    fs::remove_all(root, ec);

    std::string error;

    // A complete session with one press: 100 blocks, presses counted in block 75.
    writeSession(root / "good", true, true);
    auto good = ii_analyzer::analyzeSession(root / "good", error);
    assert(good && error.empty());
    assert(good->frameCount == 200);
    assert(good->levelName == "Acu");
    assert(good->unsupportedFrames == 100);
    assert(good->blocks.size() == ii_analyzer::kBlockCount);
    assert(good->pressCount == 1 && good->releaseCount == 1 && good->unmatchedInputs == 0);
    // Frames 150 and 151 are both in block 150*100/200 = 75, so the press and the
    // release both land there. Every other block has no input.
    assert(good->blocks[75].presses == 1);
    assert(good->blocks[75].releases == 1);
    assert(good->blocks[75].firstFrame == 150 && good->blocks[75].lastFrame == 151);
    assert(good->blocks[74].presses == 0 && good->blocks[76].presses == 0);
    assert(good->blocks[0].firstFrame == 0 && good->blocks[99].lastFrame == 199);
    // Clearance: 3 px in every frame that has the player.
    assert(good->clearance.meanPx.status == image_parser::EvidenceStatus::Measured);
    assert(std::abs(good->clearance.meanPx.value - 3.0) < 1e-9);
    assert(good->blocks[75].pressClearanceMeanPx.status == image_parser::EvidenceStatus::Measured);
    assert(std::abs(good->blocks[75].pressClearanceMeanPx.value - 3.0) < 1e-9);
    // First blocks have no player, so they have no clearance sample.
    assert(good->blocks[0].clearance.meanPx.status == image_parser::EvidenceStatus::Missing);
    assert(good->blocks[99].clearance.meanPx.status == image_parser::EvidenceStatus::Measured);

    // Incomplete sessions are refused.
    writeSession(root / "incomplete", false, true);
    assert(!ii_analyzer::analyzeSession(root / "incomplete", error));
    assert(error == "session_not_complete");

    // A session with a missing video is refused.
    writeSession(root / "novideo", true, true);
    fs::remove(root / "novideo" / "video.mp4");
    assert(!ii_analyzer::analyzeSession(root / "novideo", error));
    assert(error == "missing_or_empty_video");

    // An empty input log is valid: no presses.
    writeSession(root / "noinputs", true, false);
    auto noInputs = ii_analyzer::analyzeSession(root / "noinputs", error);
    assert(noInputs && noInputs->pressCount == 0);

    fs::remove_all(root, ec);
    std::puts("I/I Analyzer v2 tests passed");
    return 0;
}
