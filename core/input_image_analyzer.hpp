// GeoDan Input/Image Analyzer (I/I Analyzer) - portable core, schema v2.
// Reads one MP4-storage recorder session (manifest v2, frames.csv, hitboxes.csv,
// inputs.csv, video.mp4) and produces 100 blocks of evidence: the player's
// clearance to the nearest hazard while in active gameplay, and the presses that
// fell in each block. The video file is checked to exist; pixels are not decoded here.
#pragma once

#include "image_parser.hpp"
#include "input_motion_check.hpp"
#include "input_session.hpp"
#include "recorder_drops.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace geodan::core::ii_analyzer {

inline constexpr std::size_t kBlockCount = 100;

struct Block {
    std::uint32_t index = 0;
    std::uint64_t firstFrame = 0; // inclusive frame ordinal
    std::uint64_t lastFrame = 0;  // inclusive frame ordinal
    image_parser::ClearanceStatistics clearance;
    std::uint32_t presses = 0, releases = 0;
    // Index 0 is player 1, index 1 is player 2. Single-player runs count everything as player 1.
    std::array<std::uint32_t, 2> playerPresses{}, playerReleases{};
    image_parser::Metric pressClearanceMeanPx;
};

// Close-approach thresholds in reference pixels (360 px wide). Provisional values.
inline constexpr std::array<double, 3> kCloseThresholdsPx = {5.0, 10.0, 20.0};

// A section is about 5 seconds of frames (fps * 5). Close approaches are counted as
// episodes: a run of frames below a threshold counts once, when it starts.
struct Section {
    std::uint32_t index = 0;
    std::uint64_t firstFrame = 0; // inclusive frame ordinal
    std::uint64_t lastFrame = 0;  // inclusive frame ordinal
    std::uint32_t presses = 0, releases = 0;
    std::array<std::uint32_t, 2> playerPresses{}, playerReleases{};
    image_parser::ClearanceStatistics clearance;
    std::array<std::uint32_t, kCloseThresholdsPx.size()> closeApproaches{};
};

struct Result {
    std::string levelName = "unknown";     // from the manifest; "unknown" when null or absent
    std::uint64_t frameCount = 0;          // every recorded frame
    // The end screen: trailing frames where no player moved (found from states.csv).
    // Everything below is counted over analyzedFrames only; a session without
    // states.csv has no end screen found and analyzedFrames == frameCount.
    std::uint64_t analyzedFrames = 0;
    std::uint64_t endScreenFrames = 0;
    std::uint64_t endScreenInputs = 0;     // presses and releases made on the end screen, not counted
    std::uint64_t unsupportedFrames = 0;   // hitbox frames with no usable geometry
    std::uint64_t inactiveFrames = 0;      // frames outside active gameplay
    std::uint64_t pressCount = 0, releaseCount = 0;
    bool dualMode = false;                 // from the manifest; players are split only when true
    std::array<std::uint64_t, 2> playerPressCount{}, playerReleaseCount{};
    std::uint64_t unmatchedInputs = 0;     // events that could not be given a frame (always 0 since A9)
    // Capture losses from capture_drops.csv. A session without the file predates it:
    // dropsReported is false and the counts are unknown (zero).
    bool dropsReported = false;
    bool lossy = false;                    // any input, touch or frame was dropped
    std::uint64_t inputsDropped = 0, touchesDropped = 0, noBufferDrops = 0;
    // Substage 2: presses and trajectory deviations per player, from states.csv and
    // touches.csv. motionKnown is false when states.csv is missing or has no height column.
    bool motionKnown = false;
    std::array<motion::PlayerMotion, 2> motion;
    image_parser::ClearanceStatistics clearance;
    std::vector<Block> blocks;
    std::vector<Section> sections;
};

inline bool readText(const std::filesystem::path& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

inline std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string cell;
    std::istringstream cells(line);
    while (std::getline(cells, cell, ',')) fields.push_back(cell);
    if (!line.empty() && line.back() == ',') fields.emplace_back(); // trailing empty cell
    return fields;
}

// True when states.csv has a row with player 2 values (the world_x_p2 column is
// non-empty). The column is found by its header name, so column order can change.
inline bool statesHavePlayer2(const std::filesystem::path& path) {
    std::string text;
    if (!readText(path, text)) return false;
    std::istringstream lines(text);
    std::string line;
    if (!std::getline(lines, line)) return false;
    const auto header = splitCsvLine(line);
    const auto column = std::find(header.begin(), header.end(), "world_x_p2");
    if (column == header.end()) return false;
    const auto index = static_cast<std::size_t>(std::distance(header.begin(), column));
    while (std::getline(lines, line)) {
        const auto fields = splitCsvLine(line);
        if (index < fields.size() && !fields[index].empty()) return true;
    }
    return false;
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

// The frame that first shows an input made at time t: the first frame stamped at or
// after t. A frame is stamped when it is captured, and an input handled after that
// stamp appears in the next frame. An input after the last stamp (still inside the
// session) belongs to the last frame. Frames must not be empty.
inline std::size_t frameShowingEvent(const std::vector<image_parser::FrameStamp>& frames,
                                     std::uint64_t t) {
    const auto it = std::lower_bound(frames.begin(), frames.end(), t,
        [](const image_parser::FrameStamp& s, std::uint64_t value) { return s.timestampNs < value; });
    if (it == frames.end()) return frames.size() - 1;
    return static_cast<std::size_t>(std::distance(frames.begin(), it));
}

// How many leading frames are gameplay. The end screen (or a death screen) is a tail
// of frames where no player's world position changes. A tail shorter than half a
// second is ignored, and so is a session where nothing ever moved. Returns the frame
// count when states.csv is missing or has no position columns.
inline std::size_t activeFrameCount(const std::filesystem::path& statesPath,
                                    const std::vector<image_parser::FrameStamp>& frames,
                                    std::uint32_t fps) {
    const std::size_t total = frames.size();
    std::string text;
    if (!readText(statesPath, text)) return total;
    std::istringstream lines(text);
    std::string line;
    if (!std::getline(lines, line)) return total;
    const auto header = splitCsvLine(line);
    const auto columnOf = [&](const std::string& name) -> std::optional<std::size_t> {
        const auto it = std::find(header.begin(), header.end(), name);
        if (it == header.end()) return std::nullopt;
        return static_cast<std::size_t>(std::distance(header.begin(), it));
    };
    const auto sequenceColumn = columnOf("sequence");
    const auto xColumn = columnOf("world_x");
    const auto yColumn = columnOf("world_y");
    if (!sequenceColumn || !xColumn || !yColumn) return total;
    std::vector<std::size_t> positionColumns = {*xColumn, *yColumn};
    if (const auto x2 = columnOf("world_x_p2")) positionColumns.push_back(*x2);
    if (const auto y2 = columnOf("world_y_p2")) positionColumns.push_back(*y2);

    std::unordered_map<std::uint64_t, std::size_t> indexOfSequence;
    indexOfSequence.reserve(total);
    for (std::size_t i = 0; i < total; ++i) indexOfSequence.emplace(frames[i].sequence, i);

    std::vector<std::string> previous;
    bool havePrevious = false;
    std::optional<std::size_t> lastMove;
    while (std::getline(lines, line)) {
        const auto fields = splitCsvLine(line);
        if (*sequenceColumn >= fields.size()) continue;
        char* endPtr = nullptr;
        const auto sequence = std::strtoull(fields[*sequenceColumn].c_str(), &endPtr, 10);
        if (endPtr == fields[*sequenceColumn].c_str() || *endPtr != '\0') continue;
        const auto found = indexOfSequence.find(sequence);
        if (found == indexOfSequence.end()) continue;
        std::vector<std::string> current;
        for (const auto column : positionColumns)
            current.push_back(column < fields.size() ? fields[column] : std::string());
        if (havePrevious && current != previous) lastMove = found->second;
        previous = std::move(current);
        havePrevious = true;
    }
    if (!lastMove) return total;
    const std::size_t tail = total - 1 - *lastMove;
    return tail >= static_cast<std::size_t>(fps / 2) ? *lastMove + 1 : total;
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
    // Geometry is looked up by sequence number (one hash lookup per frame).
    std::unordered_map<std::uint64_t, const image_parser::GeometryFrame*> geometryBySequence;
    geometryBySequence.reserve(geometry->size());
    for (const auto& g : *geometry) geometryBySequence.emplace(g.sequence, &g);
    if (geometryBySequence.size() != geometry->size()) { error = "duplicate_frame_geometry"; return std::nullopt; }
    std::vector<const image_parser::GeometryFrame*> byIndex;
    byIndex.reserve(frames->size());
    for (const auto& stamp : *frames) {
        const auto match = geometryBySequence.find(stamp.sequence);
        if (match == geometryBySequence.end()) { error = "missing_frame_geometry"; return std::nullopt; }
        byIndex.push_back(match->second);
    }

    const std::uint64_t frameIntervalNs = 1'000'000'000ull / fps;
    const std::uint64_t sessionEndNs = frames->back().timestampNs + frameIntervalNs;
    const auto events = input_session::readInputCsv(session / "inputs.csv", error);
    if (!events) return std::nullopt;
    // The manifest flag is read when recording starts, so a level that turns on
    // dual mode later shows only in states.csv (player 2 values on those frames).
    const bool dualMode = manifestJson.find("\"dual_mode\": true") != std::string::npos
                          || statesHavePlayer2(session / "states.csv");
    if (!input_session::checkInputs(*events, sessionEndNs, error, dualMode)) return std::nullopt;

    // Clearance per frame, in reference pixels (360 px wide). Frames without a
    // usable player or hazard set have no sample.
    const double scale = image_parser::kReferenceCaptureWidth / static_cast<double>(width);
    Result result;
    {
        std::string dropsText;
        if (readText(session / "capture_drops.csv", dropsText)) {
            recorder::DropCounts drops;
            if (!recorder::parseDropsCsv(dropsText, drops)) { error = "capture_drops_invalid"; return std::nullopt; }
            result.dropsReported = true;
            result.inputsDropped = drops.inputsDropped;
            result.touchesDropped = drops.touchesDropped;
            result.noBufferDrops = drops.noBufferDrops;
            result.lossy = drops.any();
        }
    }
    result.dualMode = dualMode;
    result.levelName = levelName ? *levelName : std::string("unknown");
    result.frameCount = frames->size();
    // Frames after the last movement are the end screen; they are not gameplay.
    const std::size_t active = activeFrameCount(session / "states.csv", *frames, fps);
    result.analyzedFrames = active;
    result.endScreenFrames = frames->size() - active;
    std::vector<std::optional<double>> frameClearance(frames->size());
    std::vector<double> allClearances;
    for (std::size_t i = 0; i < active; ++i) {
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
    const std::size_t blockCount = std::min<std::size_t>(kBlockCount, active);
    const auto blockOf = [&](std::size_t frameIndex) {
        return frameIndex * blockCount / active;
    };
    result.blocks.resize(blockCount);
    std::vector<std::vector<double>> blockClearances(blockCount);
    std::vector<std::vector<double>> blockPressClearances(blockCount);
    for (std::size_t b = 0; b < blockCount; ++b) {
        result.blocks[b].index = static_cast<std::uint32_t>(b);
    }
    for (std::size_t i = 0; i < active; ++i) {
        const auto b = blockOf(i);
        if (i == 0 || blockOf(i - 1) != b) result.blocks[b].firstFrame = i;
        result.blocks[b].lastFrame = i;
        if (frameClearance[i]) blockClearances[b].push_back(*frameClearance[i]);
    }

    // Sections of about 5 seconds, counted in frames at the declared fps.
    const std::size_t sectionFrames = static_cast<std::size_t>(fps) * 5;
    const std::size_t sectionCount = (active + sectionFrames - 1) / sectionFrames;
    result.sections.resize(sectionCount);
    std::vector<std::vector<double>> sectionClearances(sectionCount);
    for (std::size_t s = 0; s < sectionCount; ++s) result.sections[s].index = static_cast<std::uint32_t>(s);
    for (std::size_t i = 0; i < active; ++i) {
        auto& section = result.sections[i / sectionFrames];
        if (i % sectionFrames == 0) section.firstFrame = i;
        section.lastFrame = i;
        if (frameClearance[i]) sectionClearances[i / sectionFrames].push_back(*frameClearance[i]);
    }

    for (const auto& e : *events) {
        const std::size_t frame = frameShowingEvent(*frames, e.timeNs);
        if (frame >= active) { ++result.endScreenInputs; continue; }
        auto& block = result.blocks[blockOf(frame)];
        auto& section = result.sections[frame / sectionFrames];
        const std::size_t player = (dualMode && !e.player1) ? 1 : 0;
        if (e.down) {
            ++block.presses;
            ++section.presses;
            ++block.playerPresses[player];
            ++section.playerPresses[player];
            ++result.playerPressCount[player];
            ++result.pressCount;
            if (frameClearance[frame]) blockPressClearances[blockOf(frame)].push_back(*frameClearance[frame]);
        } else {
            ++block.releases;
            ++section.releases;
            ++block.playerReleases[player];
            ++section.playerReleases[player];
            ++result.playerReleaseCount[player];
            ++result.releaseCount;
        }
    }

    // Substage 2: check presses against the recorded motion.
    {
        std::string statesText;
        std::vector<std::uint64_t> sequences;
        sequences.reserve(frames->size());
        for (const auto& stamp : *frames) sequences.push_back(stamp.sequence);
        if (readText(session / "states.csv", statesText)) {
            if (const auto tracks = motion::readStateSamples(statesText, sequences)) {
                std::array<std::vector<std::size_t>, 2> pressFrames, touchFrames;
                for (const auto& e : *events) {
                    if (!e.down) continue;
                    pressFrames[(dualMode && !e.player1) ? 1 : 0].push_back(frameShowingEvent(*frames, e.timeNs));
                }
                std::string touchText;
                if (readText(session / "touches.csv", touchText)) {
                    for (const auto& t : motion::readTouchTimes(touchText))
                        touchFrames[(dualMode && !t.player1) ? 1 : 0].push_back(frameShowingEvent(*frames, t.timeNs));
                }
                for (std::size_t p = 0; p < 2; ++p)
                    result.motion[p] = motion::analyzePlayer((*tracks)[p], pressFrames[p], touchFrames[p], active);
                result.motionKnown = true;
            }
        }
    }

    for (std::size_t b = 0; b < blockCount; ++b) {
        auto& block = result.blocks[b];
        block.clearance = image_parser::clearanceStatistics(blockClearances[b]);
        const auto press = image_parser::clearanceStatistics(blockPressClearances[b]);
        block.pressClearanceMeanPx = press.meanPx;
    }

    for (std::size_t s = 0; s < sectionCount; ++s)
        result.sections[s].clearance = image_parser::clearanceStatistics(sectionClearances[s]);

    // Close-approach episodes per section and threshold.
    std::array<bool, kCloseThresholdsPx.size()> inside{};
    for (std::size_t i = 0; i < active; ++i) {
        auto& section = result.sections[i / sectionFrames];
        for (std::size_t t = 0; t < kCloseThresholdsPx.size(); ++t) {
            const bool now = frameClearance[i] && *frameClearance[i] < kCloseThresholdsPx[t];
            if (now && !inside[t]) ++section.closeApproaches[t];
            inside[t] = now;
        }
    }
    error.clear();
    return result;
}

} // namespace geodan::core::ii_analyzer
