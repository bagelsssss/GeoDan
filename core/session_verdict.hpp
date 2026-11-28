// I/I Analyzer substage 3: can this session be trusted as evidence?
//
// Reads one recorded session and returns a verdict with the reasons, so the case
// file handed to the Dan Analyzer never carries a problem that nobody reported.
//
//   trusted   nothing worth reporting beyond notes
//   degraded  usable, but something was lost, cut short or out of range
//   rejected  the session contradicts itself or cannot be read
//
// Notes are facts the judge should know (an end screen was trimmed, a start delay,
// presses with no takeoff) and never lower the verdict. The thresholds below are
// named constants so they can be changed in one place.
//
// The video is not decoded here. The caller may pass the frame count it measured;
// without it the video check is skipped and noted. Pass the container's frame
// count (ffprobe nb_frames or packet count), not a decoded count: a decoder can
// drop the last frame, as happened when counting session d by decoding.
#pragma once

#include "image_parser.hpp"
#include "input_image_analyzer.hpp"
#include "input_motion_check.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace geodan::core::verdict {

// A frame this many times longer than the frame interval counts as a hitch.
inline constexpr double kHitchFactor = 2.0;
// More hitches than this share of the frames degrades the session.
inline constexpr double kHitchShare = 0.02;
// Unsupported frames beyond the start delay, as a share of analyzed frames.
inline constexpr double kUnsupportedShare = 0.10;
// Frames where the player is not a cube, as a share of analyzed frames.
inline constexpr double kNonCubeShare = 0.50;

enum class Level { Trusted, Degraded, Rejected };
enum class Severity { Note, Degraded, Rejected };

struct Finding {
    Severity severity = Severity::Note;
    std::string code;
    std::string detail;
};

struct Options {
    std::optional<std::uint64_t> videoFrames;   // measured by the caller, if known
};

struct Verdict {
    Level level = Level::Trusted;
    std::vector<Finding> findings;
    bool has(const std::string& code) const {
        for (const auto& f : findings) if (f.code == code) return true;
        return false;
    }
    std::size_t count(Severity severity) const {
        return static_cast<std::size_t>(std::count_if(findings.begin(), findings.end(),
            [&](const Finding& f) { return f.severity == severity; }));
    }
};

inline const char* levelName(Level level) {
    switch (level) {
        case Level::Trusted: return "trusted";
        case Level::Degraded: return "degraded";
        case Level::Rejected: return "rejected";
    }
    return "rejected";
}

inline Verdict verifySession(const std::filesystem::path& session, const Options& options = {}) {
    Verdict out;
    const auto add = [&](Severity severity, std::string code, std::string detail) {
        out.findings.push_back({severity, std::move(code), std::move(detail)});
    };
    const auto finish = [&]() {
        out.level = Level::Trusted;
        for (const auto& f : out.findings) {
            if (f.severity == Severity::Rejected) { out.level = Level::Rejected; break; }
            if (f.severity == Severity::Degraded) out.level = Level::Degraded;
        }
        return out;
    };

    // The analyzer already refuses a session that contradicts its manifest.
    std::string error;
    const auto result = ii_analyzer::analyzeSession(session, error);
    if (!result) {
        add(Severity::Rejected, error.empty() ? "analysis_failed" : error, "the analyzer could not read the session");
        return finish();
    }

    std::string manifest;
    ii_analyzer::readText(session / "manifest.json", manifest);
    const auto field = [&](const char* name) {
        return image_parser::jsonUnsignedField(manifest, name).value_or(0);
    };
    const auto fps = std::max<std::uint64_t>(1, field("target_fps"));
    const auto written = field("frames_written");

    // Video: the frame count the caller measured must match the metadata.
    if (options.videoFrames) {
        if (*options.videoFrames != written)
            add(Severity::Rejected, "video_frame_mismatch",
                "video has " + std::to_string(*options.videoFrames) + " frames, manifest says " + std::to_string(written));
    } else {
        add(Severity::Note, "video_not_checked", "no video frame count was supplied");
    }

    // How the recording ended.
    const auto stop = image_parser::jsonStringField(manifest, "stop_reason").value_or("");
    if (stop != "level_complete")
        add(Severity::Degraded, "level_not_finished", "stop reason was '" + stop + "', not level_complete");

    // Capture losses.
    if (field("missed_capture_intervals") > 0)
        add(Severity::Degraded, "missed_capture_intervals", std::to_string(field("missed_capture_intervals")) + " capture intervals missed");
    if (field("writer_queue_drops") > 0)
        add(Severity::Degraded, "writer_queue_drops", std::to_string(field("writer_queue_drops")) + " frames dropped by the writer");
    if (!result->dropsReported)
        add(Severity::Note, "drops_not_reported", "capture_drops.csv is absent, so input and touch losses are unknown");
    else if (result->lossy)
        add(Severity::Degraded, "events_dropped",
            std::to_string(result->inputsDropped) + " inputs, " + std::to_string(result->touchesDropped) + " touches, "
            + std::to_string(result->noBufferDrops) + " frames without a buffer");

    // Frame timing: count hitches.
    std::string frameError;
    const auto frames = image_parser::readFrameCsv(session / "frames.csv", frameError);
    if (frames && frames->size() > 1) {
        const double interval = 1e9 / static_cast<double>(fps);
        std::uint64_t hitches = 0;
        for (std::size_t i = 1; i < frames->size(); ++i)
            if (static_cast<double>((*frames)[i].deltaNs) > kHitchFactor * interval) ++hitches;
        if (hitches > 0) {
            const bool many = static_cast<double>(hitches) > kHitchShare * static_cast<double>(frames->size());
            add(many ? Severity::Degraded : Severity::Note, "frame_hitches",
                std::to_string(hitches) + " frames longer than " + std::to_string(static_cast<int>(kHitchFactor)) + "x the frame interval");
        }
    }

    // Player state file.
    std::string states;
    const bool haveStates = ii_analyzer::readText(session / "states.csv", states);
    if (!haveStates) {
        add(Severity::Degraded, "no_states", "states.csv is missing, so motion could not be checked");
    } else if (frames) {
        std::vector<std::uint64_t> sequences;
        for (const auto& f : *frames) sequences.push_back(f.sequence);
        const auto tracks = motion::readStateSamples(states, sequences);
        if (!tracks) {
            add(Severity::Degraded, "states_unreadable", "states.csv has no height column");
        } else {
            // Start delay: frames before the first player moves.
            const auto& p1 = (*tracks)[0];
            std::size_t firstMove = p1.size();
            for (std::size_t i = 1; i < p1.size(); ++i)
                if (p1[i].valid && p1[0].valid && p1[i].x != p1[0].x) { firstMove = i; break; }
            if (firstMove > 0 && firstMove < p1.size())
                add(Severity::Note, "start_delay", std::to_string(firstMove) + " frames before the player first moves");
            // Modes other than cube are not judged.
            std::uint64_t nonCube = 0, judged = 0;
            for (const auto& track : *tracks)
                for (std::size_t i = 0; i < std::min<std::size_t>(track.size(), result->analyzedFrames); ++i)
                    if (track[i].valid) { ++judged; if (!track[i].cube) ++nonCube; }
            if (judged > 0 && nonCube > 0)
                add(static_cast<double>(nonCube) > kNonCubeShare * static_cast<double>(judged) ? Severity::Degraded : Severity::Note,
                    "non_cube_frames", std::to_string(nonCube) + " of " + std::to_string(judged) + " player frames are not cube and are not judged");
        }
    }

    // Unsupported frames beyond the leading start delay.
    if (result->analyzedFrames > 0 && result->unsupportedFrames > 0) {
        std::uint64_t leading = 0;
        {
            std::string hitboxText;
            if (ii_analyzer::readText(session / "hitboxes.csv", hitboxText)) {
                // Count the leading run of unsupported frame rows.
                std::istringstream lines(hitboxText);
                std::string line;
                std::getline(lines, line);
                bool inRun = true;
                while (inRun && std::getline(lines, line)) {
                    const auto cells = motion::splitFields(line);
                    if (cells.size() < 4 || cells[3] != "frame") continue;
                    if (cells[2] == "unsupported") ++leading; else inRun = false;
                }
            }
        }
        const std::uint64_t beyond = result->unsupportedFrames > leading ? result->unsupportedFrames - leading : 0;
        if (leading > 0) add(Severity::Note, "leading_unsupported", std::to_string(leading) + " unsupported frames at the start");
        if (beyond > 0) {
            const bool many = static_cast<double>(beyond) > kUnsupportedShare * static_cast<double>(result->analyzedFrames);
            add(many ? Severity::Degraded : Severity::Note, "unsupported_frames",
                std::to_string(beyond) + " unsupported frames after the start");
        }
    }

    // Facts for the judge.
    if (result->endScreenFrames > 0)
        add(Severity::Note, "end_screen_trimmed", std::to_string(result->endScreenFrames) + " end-screen frames set aside");
    if (result->endScreenInputs > 0)
        add(Severity::Note, "end_screen_inputs", std::to_string(result->endScreenInputs) + " inputs made on the end screen set aside");
    if (result->motionKnown) {
        std::uint64_t noTakeoff = 0, unexplained = 0;
        for (const auto& m : result->motion) {
            noTakeoff += m.noTakeoffPresses;
            unexplained += m.deviationsByCause[static_cast<std::size_t>(motion::Cause::Unexplained)];
        }
        if (noTakeoff > 0) add(Severity::Note, "presses_without_takeoff", std::to_string(noTakeoff) + " presses with no takeoff");
        if (unexplained > 0) add(Severity::Note, "unexplained_deviations", std::to_string(unexplained) + " trajectory deviations with no press, touch or contact");
    }
    return finish();
}

} // namespace geodan::core::verdict
