// GeoDan Input/Image Analyzer (I/I Analyzer) - portable core, schema v2.
// Reads one MP4-storage recorder session (manifest v2, frames.csv, hitboxes.csv,
// inputs.csv, video.mp4) and produces 100 blocks of evidence: the player's
// clearance to the nearest hazard while in active gameplay, and the presses that
// fell in each block. The video file is checked to exist; pixels are not decoded here.
#pragma once

#include "image_parser.hpp"
#include "input_session.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace geodan::core::ii_analyzer {

inline constexpr std::size_t kBlockCount = 100;

struct Block {
    std::uint32_t index = 0;
    std::uint64_t firstFrame = 0; // inclusive frame ordinal
    std::uint64_t lastFrame = 0;  // inclusive frame ordinal
    image_parser::ClearanceStatistics clearance;
    std::uint32_t presses = 0, releases = 0;
    image_parser::Metric pressClearanceMeanPx;
};

struct Result {
    std::string levelName = "unknown";     // from the manifest; "unknown" when null or absent
    std::uint64_t frameCount = 0;
    std::uint64_t unsupportedFrames = 0;   // hitbox frames with no usable geometry
    std::uint64_t inactiveFrames = 0;      // frames outside active gameplay
    std::uint64_t pressCount = 0, releaseCount = 0;
    std::uint64_t unmatchedInputs = 0;     // events with no frame at or before them
    image_parser::ClearanceStatistics clearance;
    std::vector<Block> blocks;
};

inline bool readText(const std::filesystem::path& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

// Checks the v2 manifest contract and returns the frame rate and size it declares.
inline bool checkManifestV2(const std::filesystem::path& session, std::uint32_t& width,
                            std::uint32_t& height, std::uint32_t& fps, std::uint64_t& framesWritten,
                            std::string& error) {
    std::string json;
    if (!readText(session / "manifest.json", json)) { error = "cannot_open_manifest"; return false; }
    const auto version = image_parser::jsonUnsignedField(json, "schema_version");
    if (!version || *version != 2) { error = "manifest_not_schema_v2"; return false; }
    const auto status = image_parser::jsonStringField(json, "status");
    if (!status || *status != "complete") { error = "session_not_complete"; return false; }
    const auto storage = image_parser::jsonStringField(json, "frame_storage");
    const auto video = image_parser::jsonStringField(json, "video_file");
    const auto inputs = image_parser::jsonStringField(json, "input_file");
    const auto metadata = image_parser::jsonStringField(json, "frames_metadata");
    if (!storage || *storage != "mp4" || !video || *video != "video.mp4" ||
        !inputs || *inputs != "inputs.csv" || !metadata || *metadata != "frames.csv") {
        error = "manifest_paths_invalid"; return false;
    }
    const auto w = image_parser::jsonUnsignedField(json, "width");
    const auto h = image_parser::jsonUnsignedField(json, "height");
    const auto f = image_parser::jsonUnsignedField(json, "target_fps");
    const auto written = image_parser::jsonUnsignedField(json, "frames_written");
    if (!w || !h || !f || !written || *w == 0 || *h == 0 || *f == 0) {
        error = "manifest_missing_or_invalid_field"; return false;
    }
    width = static_cast<std::uint32_t>(*w);
    height = static_cast<std::uint32_t>(*h);
    fps = static_cast<std::uint32_t>(*f);
    framesWritten = *written;
    std::error_code ec;
    const auto size = std::filesystem::file_size(session / "video.mp4", ec);
    if (ec || size == 0) { error = "missing_or_empty_video"; return false; }
    return true;
}

// Index of the last frame whose timestamp is at or before t, or nullopt if t is
// before the first frame.
inline std::optional<std::size_t> frameAtOrBefore(const std::vector<image_parser::FrameStamp>& frames,
                                                  std::uint64_t t) {
    auto it = std::upper_bound(frames.begin(), frames.end(), t,
        [](std::uint64_t value, const image_parser::FrameStamp& s) { return value < s.timestampNs; });
    if (it == frames.begin()) return std::nullopt;
    return static_cast<std::size_t>(std::distance(frames.begin(), it) - 1);
}

inline std::optional<Result> analyzeSession(const std::filesystem::path& session, std::string& error) {
    std::uint32_t width = 0, height = 0, fps = 0;
    std::uint64_t framesWritten = 0;
    if (!checkManifestV2(session, width, height, fps, framesWritten, error)) return std::nullopt;
    std::string manifestJson;
    readText(session / "manifest.json", manifestJson);
    const auto levelName = image_parser::jsonStringField(manifestJson, "level_name");

    const auto frames = image_parser::readFrameCsv(session / "frames.csv", error);
    if (!frames) return std::nullopt;
    if (frames->empty() || frames->size() != framesWritten) { error = "frames_written_count_mismatch"; return std::nullopt; }

    const auto geometry = image_parser::readGeometryCsv(session / "hitboxes.csv", error);
    if (!geometry) return std::nullopt;
    if (geometry->size() != frames->size()) { error = "geometry_frame_count_mismatch"; return std::nullopt; }
    std::vector<const image_parser::GeometryFrame*> byIndex;
    byIndex.reserve(frames->size());
    for (const auto& stamp : *frames) {
        const auto match = std::find_if(geometry->begin(), geometry->end(),
            [&](const image_parser::GeometryFrame& g) { return g.sequence == stamp.sequence; });
        if (match == geometry->end()) { error = "missing_frame_geometry"; return std::nullopt; }
        byIndex.push_back(&*match);
    }

    const std::uint64_t frameIntervalNs = 1'000'000'000ull / fps;
    const std::uint64_t sessionEndNs = frames->back().timestampNs + frameIntervalNs;
    const auto events = input_session::readInputCsv(session / "inputs.csv", error);
    if (!events) return std::nullopt;
    if (!input_session::checkInputs(*events, sessionEndNs, error)) return std::nullopt;

    // Clearance per frame, in reference pixels (360 px wide). Frames without a
    // usable player or hazard set have no sample.
    const double scale = image_parser::kReferenceCaptureWidth / static_cast<double>(width);
    Result result;
    result.levelName = levelName ? *levelName : std::string("unknown");
    result.frameCount = frames->size();
    std::vector<std::optional<double>> frameClearance(frames->size());
    std::vector<double> allClearances;
    for (std::size_t i = 0; i < frames->size(); ++i) {
        const auto& g = *byIndex[i];
        if (g.status == image_parser::GeometryStatus::Unsupported) { ++result.unsupportedFrames; continue; }
        if (g.phase != image_parser::Phase::ActiveGameplay) { ++result.inactiveFrames; continue; }
        std::vector<const image_parser::Rect*> players, hazards;
        for (const auto& e : g.entities) {
            if (e.kind == image_parser::EntityKind::Player) players.push_back(&e.bounds);
            if (e.kind == image_parser::EntityKind::Hazard) hazards.push_back(&e.bounds);
        }
        if (players.empty() || hazards.empty()) continue;
        double closest = std::numeric_limits<double>::infinity();
        for (const auto* p : players)
            for (const auto* h : hazards)
                closest = std::min(closest, image_parser::rectangleClearance(*p, *h) * scale);
        if (std::isfinite(closest)) {
            frameClearance[i] = closest;
            allClearances.push_back(closest);
        }
    }
    result.clearance = image_parser::clearanceStatistics(allClearances,
                                                         result.unsupportedFrames > 0);

    // 100 blocks of frames (fewer if the session has fewer frames).
    const std::size_t blockCount = std::min<std::size_t>(kBlockCount, frames->size());
    const auto blockOf = [&](std::size_t frameIndex) {
        return frameIndex * blockCount / frames->size();
    };
    result.blocks.resize(blockCount);
    std::vector<std::vector<double>> blockClearances(blockCount);
    std::vector<std::vector<double>> blockPressClearances(blockCount);
    for (std::size_t b = 0; b < blockCount; ++b) {
        result.blocks[b].index = static_cast<std::uint32_t>(b);
    }
    for (std::size_t i = 0; i < frames->size(); ++i) {
        const auto b = blockOf(i);
        if (i == 0 || blockOf(i - 1) != b) result.blocks[b].firstFrame = i;
        result.blocks[b].lastFrame = i;
        if (frameClearance[i]) blockClearances[b].push_back(*frameClearance[i]);
    }

    for (const auto& e : *events) {
        const auto frame = frameAtOrBefore(*frames, e.timeNs);
        if (!frame) { ++result.unmatchedInputs; continue; }
        auto& block = result.blocks[blockOf(*frame)];
        if (e.down) {
            ++block.presses;
            ++result.pressCount;
            if (frameClearance[*frame]) blockPressClearances[blockOf(*frame)].push_back(*frameClearance[*frame]);
        } else {
            ++block.releases;
            ++result.releaseCount;
        }
    }

    for (std::size_t b = 0; b < blockCount; ++b) {
        auto& block = result.blocks[b];
        block.clearance = image_parser::clearanceStatistics(blockClearances[b]);
        const auto press = image_parser::clearanceStatistics(blockPressClearances[b]);
        block.pressClearanceMeanPx = press.meanPx;
    }
    error.clear();
    return result;
}

} // namespace geodan::core::ii_analyzer
