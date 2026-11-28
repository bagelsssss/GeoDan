// Portable test for the v2 I/I Analyzer. Builds a synthetic session on disk, then
// analyzes it. Build:
// g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. tests/test_input_image_analyzer.cpp
#include "input_image_analyzer.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iterator>

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
        in << 150 * frameNs << ",1,1,1\n";       // press at the stamp of frame 150
        in << 151 * frameNs << ",0,1,1\n";       // release at the stamp of frame 151
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

    // Sections: 200 frames at 60 fps is one 5-second section (300 frames).
    assert(good->sections.size() == 1);
    assert(good->sections[0].firstFrame == 0 && good->sections[0].lastFrame == 199);
    assert(good->sections[0].presses == 1 && good->sections[0].releases == 1);
    // Clearance is 3 px from frame 100 on: one close-approach episode per threshold.
    for (const auto count : good->sections[0].closeApproaches) assert(count == 1);
    assert(std::abs(good->sections[0].clearance.meanPx.value - 3.0) < 1e-9);

    // Dual mode: player 2 presses and releases are counted separately.
    const std::uint64_t frameNsMain = 1'000'000'000ull / 60;
    const auto setInputs = [&](const fs::path& dir) {
        std::ofstream in(dir / "inputs.csv");
        in << "time_ns,down,button,player1\n";
        in << 150 * frameNsMain << ",1,1,1\n";  // player 1 press, frame 150
        in << 151 * frameNsMain << ",0,1,1\n";  // player 1 release, frame 151
        in << 152 * frameNsMain << ",1,1,0\n";  // player 2 press, frame 152
        in << 153 * frameNsMain << ",0,1,0\n";  // player 2 release, frame 153
    };
    writeSession(root / "dual", true, false);
    setInputs(root / "dual");
    {
        const fs::path manifest = root / "dual" / "manifest.json";
        std::ifstream read(manifest);
        std::string text((std::istreambuf_iterator<char>(read)), std::istreambuf_iterator<char>());
        const std::string marker = "\"frames_directory\": null";
        const auto at = text.find(marker);
        assert(at != std::string::npos);
        text.insert(at, "\"dual_mode\": true,\n  ");
        std::ofstream write(manifest);
        write << text;
    }
    auto dual = ii_analyzer::analyzeSession(root / "dual", error);
    assert(dual && error.empty());
    assert(dual->dualMode);
    assert(dual->pressCount == 2 && dual->releaseCount == 2);
    assert(dual->playerPressCount[0] == 1 && dual->playerPressCount[1] == 1);
    assert(dual->playerReleaseCount[0] == 1 && dual->playerReleaseCount[1] == 1);
    // Player 1 clicks on frames 150-151 (block 75); player 2 clicks on frames 152-153 (block 76).
    assert(dual->blocks[75].playerPresses[0] == 1 && dual->blocks[75].playerPresses[1] == 0);
    assert(dual->blocks[76].playerPresses[0] == 0 && dual->blocks[76].playerPresses[1] == 1);
    assert(dual->sections[0].playerPresses[1] == 1 && dual->sections[0].playerReleases[1] == 1);

    // Dual mode turned on after the manifest was written: states.csv shows player 2,
    // so the run is still treated as dual.
    writeSession(root / "late", true, false);
    setInputs(root / "late");
    {
        std::ofstream states(root / "late" / "states.csv");
        states << "sequence,mode,upside_down,gravity,world_x,world_y,mode_p2,world_x_p2,world_y_p2\n"
               << "0,cube,0,1,0,0,,,\n";
        for (int i = 1; i < 200; ++i)   // player 2 appears at frame 1; both keep moving
            states << i << ",cube,0,1," << i * 5 << ",0,cube," << i * 5 << ",0\n";
    }
    auto late = ii_analyzer::analyzeSession(root / "late", error);
    assert(late && late->dualMode);
    assert(late->playerPressCount[1] == 1);

    // The same log in a single-player run counts every click as player 1.
    writeSession(root / "single", true, false);
    setInputs(root / "single");
    auto single = ii_analyzer::analyzeSession(root / "single", error);
    assert(single && !single->dualMode);
    assert(single->playerPressCount[0] == 2 && single->playerPressCount[1] == 0);
    assert(single->playerReleaseCount[0] == 2 && single->playerReleaseCount[1] == 0);

    // States columns are found by header name, not position.
    writeSession(root / "reordered", true, false);
    setInputs(root / "reordered");
    {
        std::ofstream states(root / "reordered" / "states.csv");
        states << "sequence,world_x_p2,mode,world_x\n" << "0,,cube,1\n" << "1,7,cube,1\n";
    }
    auto reordered = ii_analyzer::analyzeSession(root / "reordered", error);
    assert(reordered && reordered->dualMode);
    // A states file with no player 2 column, or an empty one, is not dual.
    writeSession(root / "nop2", true, false);
    {
        std::ofstream states(root / "nop2" / "states.csv");
        states << "sequence,mode\n" << "0,cube\n";
    }
    auto noP2 = ii_analyzer::analyzeSession(root / "nop2", error);
    assert(noP2 && !noP2->dualMode);
    writeSession(root / "emptyp2", true, false);
    {
        std::ofstream states(root / "emptyp2" / "states.csv");
        states << "sequence,mode,world_x_p2\n" << "0,cube,\n" << "1,cube,\n";
    }
    auto emptyP2 = ii_analyzer::analyzeSession(root / "emptyp2", error);
    assert(emptyP2 && !emptyP2->dualMode);

    // Capture losses: absent file = unknown, clean file = not lossy, any drop = lossy.
    assert(!good->dropsReported && !good->lossy);
    writeSession(root / "clean", true, false);
    { std::ofstream d(root / "clean" / "capture_drops.csv");
      d << "kind,count\ninputs_dropped,0\ntouches_dropped,0\nno_buffer_drops,0\n"; }
    auto clean = ii_analyzer::analyzeSession(root / "clean", error);
    assert(clean && clean->dropsReported && !clean->lossy);
    writeSession(root / "lossy", true, false);
    { std::ofstream d(root / "lossy" / "capture_drops.csv");
      d << "kind,count\ninputs_dropped,2\ntouches_dropped,0\nno_buffer_drops,5\n"; }
    auto lossy = ii_analyzer::analyzeSession(root / "lossy", error);
    assert(lossy && lossy->lossy && lossy->inputsDropped == 2 && lossy->noBufferDrops == 5);
    writeSession(root / "baddrops", true, false);
    { std::ofstream d(root / "baddrops" / "capture_drops.csv"); d << "not a drops file\n"; }
    assert(!ii_analyzer::analyzeSession(root / "baddrops", error));
    assert(error == "capture_drops_invalid");

    // A9: an input is shown by the first frame stamped at or after it.
    writeSession(root / "shift", true, false);
    {
        std::ofstream in(root / "shift" / "inputs.csv");
        in << "time_ns,down,button,player1\n";
        in << 100 * frameNsMain + 1 << ",1,1,1\n";                  // just after frame 100 -> frame 101
        in << 102 * frameNsMain - 1 << ",0,1,1\n";                  // just before frame 102 -> frame 102
        in << 199 * frameNsMain + 10 << ",1,1,1\n";                 // after the last stamp -> last frame
    }
    auto shifted = ii_analyzer::analyzeSession(root / "shift", error);
    assert(shifted && error.empty());
    assert(shifted->pressCount == 2 && shifted->releaseCount == 1 && shifted->unmatchedInputs == 0);
    assert(shifted->blocks[50].presses == 1 && shifted->blocks[50].releases == 0);   // frame 101
    assert(shifted->blocks[51].releases == 1);                                       // frame 102
    assert(shifted->blocks[99].presses == 1);                                        // frame 199

    // A10: a tail of frames where nobody moves is the end screen.
    writeSession(root / "endscreen", true, false);
    {
        std::ofstream states(root / "endscreen" / "states.csv");
        states << "sequence,mode,upside_down,gravity,world_x,world_y,mode_p2,world_x_p2,world_y_p2\n";
        for (int i = 0; i < 200; ++i) {          // moves until frame 119, then frozen for 80 frames
            const int x = (i < 120 ? i : 119) * 5;
            states << i << ",cube,0,1," << x << ",10,,,\n";
        }
        std::ofstream in(root / "endscreen" / "inputs.csv");
        in << "time_ns,down,button,player1\n";
        in << 100 * frameNsMain << ",1,1,1\n";   // gameplay
        in << 110 * frameNsMain << ",0,1,1\n";   // gameplay
        in << 150 * frameNsMain << ",1,1,1\n";   // end screen
        in << 160 * frameNsMain << ",0,1,1\n";   // end screen
    }
    auto ended = ii_analyzer::analyzeSession(root / "endscreen", error);
    assert(ended && error.empty());
    assert(ended->frameCount == 200 && ended->analyzedFrames == 120 && ended->endScreenFrames == 80);
    assert(ended->pressCount == 1 && ended->releaseCount == 1 && ended->endScreenInputs == 2);
    assert(ended->sections.size() == 1 && ended->sections[0].lastFrame == 119);
    assert(ended->blocks.size() == ii_analyzer::kBlockCount);
    assert(ended->blocks[99].lastFrame == 119);
    // The player exists from frame 100, so only frames 100-119 have clearance.
    assert(ended->clearance.meanPx.status == image_parser::EvidenceStatus::Measured);

    // A frozen tail shorter than half a second (30 frames at 60 fps) is not an end screen.
    writeSession(root / "shorttail", true, false);
    {
        std::ofstream states(root / "shorttail" / "states.csv");
        states << "sequence,mode,upside_down,gravity,world_x,world_y,mode_p2,world_x_p2,world_y_p2\n";
        for (int i = 0; i < 200; ++i)
            states << i << ",cube,0,1," << (i < 190 ? i : 190) * 5 << ",10,,,\n";
    }
    auto shortTail = ii_analyzer::analyzeSession(root / "shorttail", error);
    assert(shortTail && shortTail->analyzedFrames == 200 && shortTail->endScreenFrames == 0);

    // A session where nothing ever moved is not trimmed to nothing.
    writeSession(root / "still", true, false);
    {
        std::ofstream states(root / "still" / "states.csv");
        states << "sequence,mode,upside_down,gravity,world_x,world_y,mode_p2,world_x_p2,world_y_p2\n";
        for (int i = 0; i < 200; ++i) states << i << ",cube,0,1,5,10,,,\n";
    }
    auto still = ii_analyzer::analyzeSession(root / "still", error);
    assert(still && still->analyzedFrames == 200);

    // A session without states.csv has no end screen found.
    assert(good->analyzedFrames == good->frameCount && good->endScreenFrames == 0);

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
