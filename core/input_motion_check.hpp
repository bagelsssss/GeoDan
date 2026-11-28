// I/I Analyzer substage 2: checks each press against the player's recorded motion.
//
// Two outputs per player, from states.csv world_y:
//  1. Press check: after a press, does the player's height rise within a few
//     frames (a takeoff)? A press with no takeoff is only counted as
//     no_takeoff. It is not an error: the player may have been in the air, the
//     press may have been buffered, or the press may have done something else.
//  2. Trajectory predictor: in free fall the height follows a quadratic arc, so
//     the next height is predicted from the last three. A frame where the real
//     height is far from the prediction is a deviation, explained by a press, a
//     touch (pad, orb, portal), a contact (the player was stopped), or left
//     unexplained (for example a moving block pushing the player).
//
// Cube only. Frames in other modes, frames without a position and the first
// frames after a gap are unknown and judged by nothing.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace geodan::core::motion {

// A height that differs from the prediction by more than this, in world units,
// is a deviation. Free fall at 60 fps predicts to about 0.01.
inline constexpr double kDeviationThreshold = 0.5;
// A press explains a takeoff or a deviation this many frames after its frame.
inline constexpr std::size_t kExplainWindow = 3;
inline constexpr std::size_t kHistoryFrames = 3;

struct Sample {
    bool valid = false;      // a position was recorded this frame
    bool cube = false;
    bool upsideDown = false;
    double x = 0.0;
    double y = 0.0;
};

enum class Cause { Press, Touch, Contact, Unexplained };

struct Deviation {
    std::size_t frame = 0;
    double residual = 0.0;   // actual height minus predicted height
    Cause cause = Cause::Unexplained;
};

struct PlayerMotion {
    std::uint64_t takeoffPresses = 0;
    std::uint64_t noTakeoffPresses = 0;
    std::uint64_t unknownPresses = 0;
    std::array<std::uint64_t, kExplainWindow + 1> takeoffOffsets{};  // frames after the assigned frame
    std::uint64_t deviations = 0;
    std::array<std::uint64_t, 4> deviationsByCause{};                // indexed by Cause
    std::vector<Deviation> deviationList;
};

inline bool usable(const std::vector<Sample>& track, std::size_t i) {
    return i < track.size() && track[i].valid && track[i].cube;
}

// True when the height changed by more than the threshold between frame i-1 and i.
// Direction does not matter, so a mirrored player 2 is handled the same way.
inline bool movesAt(const std::vector<Sample>& track, std::size_t i) {
    if (i == 0 || !usable(track, i) || !usable(track, i - 1)) return false;
    return std::fabs(track[i].y - track[i - 1].y) > kDeviationThreshold;
}

inline bool nearFrame(const std::vector<std::size_t>& frames, std::size_t frame) {
    for (const auto f : frames)
        if (f <= frame && frame - f <= kExplainWindow) return true;
    return false;
}

// pressFrames and touchFrames are frame indices for this player; only frames
// below active count.
inline PlayerMotion analyzePlayer(const std::vector<Sample>& track,
                                  const std::vector<std::size_t>& pressFrames,
                                  const std::vector<std::size_t>& touchFrames,
                                  std::size_t active) {
    PlayerMotion out;
    const std::size_t end = std::min(active, track.size());

    for (const auto f : pressFrames) {
        if (f >= end || f < kHistoryFrames || f + kExplainWindow >= end) { ++out.unknownPresses; continue; }
        bool known = true;
        for (std::size_t k = 0; k <= kExplainWindow && known; ++k) known = usable(track, f + k);
        if (!known || !usable(track, f - 1)) { ++out.unknownPresses; continue; }
        bool hit = false;
        for (std::size_t k = 0; k <= kExplainWindow; ++k) {
            if (movesAt(track, f + k) && !movesAt(track, f + k - 1)) {
                ++out.takeoffPresses;
                ++out.takeoffOffsets[k];
                hit = true;
                break;
            }
        }
        if (!hit) ++out.noTakeoffPresses;
    }

    // After a deviation the next frames predict from a history that contains it, so
    // they would deviate too: the same event echoed. They are skipped. A second,
    // separate event within three frames of the first is therefore not reported.
    std::optional<std::size_t> lastDeviation;
    for (std::size_t i = kHistoryFrames; i < end; ++i) {
        if (lastDeviation && i - *lastDeviation <= kHistoryFrames) continue;
        if (!usable(track, i) || !usable(track, i - 1) || !usable(track, i - 2) || !usable(track, i - 3)) continue;
        // Positions are sampled at uneven times (capture jitter), so the arc is
        // predicted against world x, which advances at a constant speed.
        const double x0 = track[i].x, x1 = track[i - 1].x, x2 = track[i - 2].x, x3 = track[i - 3].x;
        if (!(x0 > x1 && x1 > x2 && x2 > x3)) continue;
        const double y1 = track[i - 1].y, y2 = track[i - 2].y, y3 = track[i - 3].y;
        const double predicted = y3 * (x0 - x2) * (x0 - x1) / ((x3 - x2) * (x3 - x1))
                               + y2 * (x0 - x3) * (x0 - x1) / ((x2 - x3) * (x2 - x1))
                               + y1 * (x0 - x3) * (x0 - x2) / ((x1 - x3) * (x1 - x2));
        const double residual = track[i].y - predicted;
        if (std::fabs(residual) <= kDeviationThreshold) continue;
        Cause cause = Cause::Unexplained;
        if (nearFrame(pressFrames, i)) {
            cause = Cause::Press;
        } else if (nearFrame(touchFrames, i)) {
            cause = Cause::Touch;
        } else {
            // Stopped: moving one way and pushed back the other.
            const double velocity = y1 - y2;
            if ((velocity < 0.0 && residual > 0.0) || (velocity > 0.0 && residual < 0.0)) cause = Cause::Contact;
        }
        lastDeviation = i;
        ++out.deviations;
        ++out.deviationsByCause[static_cast<std::size_t>(cause)];
        out.deviationList.push_back({i, residual, cause});
    }
    return out;
}

// ---- reading states.csv and touches.csv ------------------------------------

inline std::vector<std::string> splitFields(const std::string& line) {
    std::vector<std::string> out;
    std::string field;
    std::stringstream stream(line);
    while (std::getline(stream, field, ',')) {
        if (!field.empty() && field.back() == '\r') field.pop_back();
        out.push_back(field);
    }
    if (!line.empty() && line.back() == ',') out.emplace_back();
    return out;
}

inline std::optional<double> parseNumber(const std::string& text) {
    if (text.empty()) return std::nullopt;
    char* endPtr = nullptr;
    const double value = std::strtod(text.c_str(), &endPtr);
    if (endPtr == text.c_str() || *endPtr != '\0') return std::nullopt;
    return value;
}

// One sample list per player, indexed by frame index. sequences[i] is the sequence
// number of frame i. Returns nothing when the file is missing or has no y column.
inline std::optional<std::array<std::vector<Sample>, 2>>
readStateSamples(const std::string& text, const std::vector<std::uint64_t>& sequences) {
    std::istringstream lines(text);
    std::string line;
    if (!std::getline(lines, line)) return std::nullopt;
    const auto header = splitFields(line);
    const auto columnOf = [&](const std::string& name) -> std::optional<std::size_t> {
        const auto it = std::find(header.begin(), header.end(), name);
        if (it == header.end()) return std::nullopt;
        return static_cast<std::size_t>(std::distance(header.begin(), it));
    };
    const auto seqCol = columnOf("sequence");
    const auto yCol = columnOf("world_y");
    if (!seqCol || !yCol) return std::nullopt;
    const auto modeCol = columnOf("mode");
    const auto flipCol = columnOf("upside_down");
    const auto xCol = columnOf("world_x");
    const auto x2Col = columnOf("world_x_p2");
    const auto y2Col = columnOf("world_y_p2");
    const auto mode2Col = columnOf("mode_p2");

    std::unordered_map<std::uint64_t, std::size_t> indexOf;
    indexOf.reserve(sequences.size());
    for (std::size_t i = 0; i < sequences.size(); ++i) indexOf.emplace(sequences[i], i);

    std::array<std::vector<Sample>, 2> tracks;
    tracks[0].assign(sequences.size(), Sample{});
    tracks[1].assign(sequences.size(), Sample{});
    const auto cell = [](const std::vector<std::string>& f, const std::optional<std::size_t>& c) {
        return (c && *c < f.size()) ? f[*c] : std::string();
    };
    while (std::getline(lines, line)) {
        const auto fields = splitFields(line);
        const auto seq = parseNumber(cell(fields, seqCol));
        if (!seq) continue;
        const auto found = indexOf.find(static_cast<std::uint64_t>(*seq));
        if (found == indexOf.end()) continue;
        const bool flipped = cell(fields, flipCol) == "1";
        if (const auto y = parseNumber(cell(fields, yCol))) {
            tracks[0][found->second] = {true, cell(fields, modeCol) == "cube", flipped,
                                        parseNumber(cell(fields, xCol)).value_or(0.0), *y};
        }
        if (const auto y = parseNumber(cell(fields, y2Col))) {
            // The upside_down column belongs to player 1; player 2's own flip is not
            // recorded, so its samples are only trusted while player 1 is upright.
            tracks[1][found->second] = {true, cell(fields, mode2Col) == "cube", flipped,
                                        parseNumber(cell(fields, x2Col)).value_or(0.0), *y};
        }
    }
    return tracks;
}

struct TouchTime {
    std::uint64_t timeNs = 0;
    bool player1 = true;
};

// touches.csv: time_ns,kind,object_type,player1. Bad rows are skipped.
inline std::vector<TouchTime> readTouchTimes(const std::string& text) {
    std::vector<TouchTime> out;
    std::istringstream lines(text);
    std::string line;
    if (!std::getline(lines, line)) return out;
    while (std::getline(lines, line)) {
        const auto fields = splitFields(line);
        if (fields.size() < 4) continue;
        const auto t = parseNumber(fields[0]);
        if (!t || *t < 0.0) continue;
        out.push_back({static_cast<std::uint64_t>(*t), fields[3] == "1"});
    }
    return out;
}

} // namespace geodan::core::motion
