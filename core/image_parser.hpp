#pragma once

// Portable, Geode-free Image Parser foundation for Recorder Session v1.
// It validates the recording session and consumes authoritative hitbox geometry.
// It never treats decorative pixels as collision geometry.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace geodan::core::image_parser {

inline constexpr std::uint32_t kRecorderSchemaVersion = 1;
inline constexpr std::size_t kImagesPerSection = 10;
inline constexpr double kReferenceCaptureWidth = 360.0;

enum class SessionStatus { Recording, Complete, Incomplete, Error, Invalid };
enum class Phase { StartIdle, ActiveGameplay, Paused, Respawn, Transition, Unknown };
enum class GeometryStatus { Valid, Unsupported };
enum class EntityKind { Frame, Player, Hazard };
enum class EvidenceStatus { Measured, Unsupported, Missing };

struct Manifest {
    std::uint32_t schemaVersion = 0;
    SessionStatus status = SessionStatus::Invalid;
    std::uint32_t width = 0, height = 0, targetFps = 0;
    std::uint64_t framesWritten = 0, missedCaptureIntervals = 0;
    std::uint64_t writerQueueDrops = 0, startUnixNs = 0;
    std::optional<std::uint64_t> endUnixNs, firstSequence, lastSequence;
    std::string framesMetadata, framesDirectory;
};

struct FrameStamp {
    std::uint64_t sequence = 0, timestampNs = 0, deltaNs = 0, missedIntervals = 0;
};

struct Rect {
    double left = 0, top = 0, right = 0, bottom = 0;
    bool valid() const {
        return std::isfinite(left) && std::isfinite(top) &&
               std::isfinite(right) && std::isfinite(bottom) &&
               left < right && top < bottom;
    }
};

struct GeometryEntity { EntityKind kind = EntityKind::Frame; std::uint64_t id = 0; Rect bounds; };
struct GeometryFrame {
    std::uint64_t sequence = 0;
    Phase phase = Phase::Unknown;
    GeometryStatus status = GeometryStatus::Unsupported;
    std::vector<GeometryEntity> entities;
};

struct Metric {
    double value = 0;
    std::uint32_t sampleCount = 0;
    EvidenceStatus status = EvidenceStatus::Missing;
};
struct ClearanceStatistics { Metric meanPx, medianPx, minimumPx, percentile10Px; };

struct Section {
    std::uint32_t index = 0;
    std::uint64_t firstSampleIndex = 0; // zero-based persisted-frame ordinal
    std::uint64_t lastSampleIndex = 0;
    Phase phase = Phase::Unknown;
    ClearanceStatistics clearance;
    Metric windows60, windows120, windows240, nerves;
    std::uint32_t unsupportedHitboxCount = 0;
};

struct ParseResult {
    bool structurallyValid = false;
    bool parserComplete = false;
    std::string error;
    std::uint64_t sourceFrameCount = 0, processedFrameCount = 0;
    std::uint64_t unclassifiedFrameCount = 0, timestampGapCount = 0;
    std::uint64_t missedCaptureIntervals = 0, writerQueueDrops = 0;
    ClearanceStatistics clearance;
    Metric windows60, windows120, windows240, nerves;
    std::vector<Section> sections;
};

inline std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

inline bool parseUnsigned(std::string_view text, std::uint64_t& value) {
    if (text.empty()) return false;
    std::uint64_t parsed = 0;
    for (const char ch : text) {
        if (ch < '0' || ch > '9') return false;
        const auto digit = static_cast<std::uint64_t>(ch - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) return false;
        parsed = parsed * 10 + digit;
    }
    value = parsed;
    return true;
}

inline std::optional<std::string> jsonStringField(const std::string& json, const std::string& key) {
    const std::string token = "\"" + key + "\"";
    const auto at = json.find(token);
    if (at == std::string::npos || json.find(token, at + token.size()) != std::string::npos) return {};
    const auto colon = json.find(':', at + token.size());
    if (colon == std::string::npos) return {};
    const auto quote = json.find('"', colon + 1);
    if (quote == std::string::npos) return {};
    const auto end = json.find('"', quote + 1);
    if (end == std::string::npos) return {};
    return json.substr(quote + 1, end - quote - 1);
}

inline std::optional<std::uint64_t> jsonUnsignedField(const std::string& json, const std::string& key) {
    const std::string token = "\"" + key + "\"";
    const auto at = json.find(token);
    if (at == std::string::npos || json.find(token, at + token.size()) != std::string::npos) return {};
    const auto colon = json.find(':', at + token.size());
    if (colon == std::string::npos) return {};
    const auto begin = json.find_first_not_of(" \t\r\n", colon + 1);
    if (begin == std::string::npos) return {};
    auto end = json.find_first_of(",}\r\n \t", begin);
    if (end == std::string::npos) end = json.size();
    std::uint64_t value = 0;
    if (!parseUnsigned(std::string_view(json).substr(begin, end - begin), value)) return {};
    return value;
}

inline bool jsonNullOrUnsignedField(const std::string& json, const std::string& key,
                                    std::optional<std::uint64_t>& output) {
    const std::string token = "\"" + key + "\"";
    const auto at = json.find(token);
    if (at == std::string::npos || json.find(token, at + token.size()) != std::string::npos) return false;
    const auto colon = json.find(':', at + token.size());
    if (colon == std::string::npos) return false;
    const auto begin = json.find_first_not_of(" \t\r\n", colon + 1);
    if (begin == std::string::npos) return false;
    if (json.compare(begin, 4, "null") == 0) { output.reset(); return true; }
    auto end = json.find_first_of(",}\r\n \t", begin);
    if (end == std::string::npos) end = json.size();
    std::uint64_t value = 0;
    if (!parseUnsigned(std::string_view(json).substr(begin, end - begin), value)) return false;
    output = value;
    return true;
}

inline std::optional<Manifest> readManifest(const std::filesystem::path& path, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) { error = "cannot_open_manifest"; return {}; }
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    Manifest m;
    const auto version = jsonUnsignedField(json, "schema_version");
    const auto status = jsonStringField(json, "status");
    const auto stopReason = jsonStringField(json, "stop_reason");
    const auto width = jsonUnsignedField(json, "width");
    const auto height = jsonUnsignedField(json, "height");
    const auto fps = jsonUnsignedField(json, "target_fps");
    const auto written = jsonUnsignedField(json, "frames_written");
    const auto missed = jsonUnsignedField(json, "missed_capture_intervals");
    const auto drops = jsonUnsignedField(json, "writer_queue_drops");
    const auto start = jsonUnsignedField(json, "start_unix_ns");
    if (!version || !status || !stopReason || !width || !height || !fps || !written || !missed ||
        !drops || !start || !jsonNullOrUnsignedField(json, "end_unix_ns", m.endUnixNs) ||
        !jsonNullOrUnsignedField(json, "first_sequence", m.firstSequence) ||
        !jsonNullOrUnsignedField(json, "last_sequence", m.lastSequence)) {
        error = "manifest_missing_or_invalid_field"; return {};
    }
    if (*version > UINT32_MAX || *width > UINT32_MAX || *height > UINT32_MAX || *fps > UINT32_MAX) {
        error = "manifest_integer_out_of_range"; return {};
    }
    m.schemaVersion = static_cast<std::uint32_t>(*version);
    m.width = static_cast<std::uint32_t>(*width);
    m.height = static_cast<std::uint32_t>(*height);
    m.targetFps = static_cast<std::uint32_t>(*fps);
    m.framesWritten = *written;
    m.missedCaptureIntervals = *missed;
    m.writerQueueDrops = *drops;
    m.startUnixNs = *start;
    if (*status == "recording") m.status = SessionStatus::Recording;
    else if (*status == "complete") m.status = SessionStatus::Complete;
    else if (*status == "incomplete") m.status = SessionStatus::Incomplete;
    else if (*status == "error") m.status = SessionStatus::Error;
    else { error = "unknown_session_status"; return {}; }
    const auto metadata = jsonStringField(json, "frames_metadata");
    const auto directory = jsonStringField(json, "frames_directory");
    if (!metadata || !directory) { error = "manifest_missing_paths"; return {}; }
    m.framesMetadata = *metadata;
    m.framesDirectory = *directory;
    if (m.schemaVersion != kRecorderSchemaVersion || !m.width || !m.height || !m.targetFps ||
        m.framesMetadata != "frames.csv" || m.framesDirectory != "frames") {
        error = "unsupported_manifest_contract"; return {};
    }
    if (m.status != SessionStatus::Complete) { error = "session_not_complete"; return {}; }
    if (!m.endUnixNs || *m.endUnixNs < m.startUnixNs) { error = "invalid_session_time_range"; return {}; }
    if ((m.framesWritten == 0) != (!m.firstSequence && !m.lastSequence) ||
        (m.framesWritten > 0 && (!m.firstSequence || !m.lastSequence ||
         *m.lastSequence < *m.firstSequence ||
         m.framesWritten > *m.lastSequence - *m.firstSequence + 1))) {
        error = "manifest_sequence_summary_invalid"; return {};
    }
    return m;
}

// Recorder Session v1 emits unquoted scalar CSV fields; quoted fields are rejected.
inline bool splitCsvLine(const std::string& line, std::vector<std::string>& fields) {
    if (line.find('"') != std::string::npos) return false;
    fields.clear();
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) fields.push_back(trim(field));
    if (!line.empty() && line.back() == ',') fields.emplace_back();
    return true;
}

inline std::optional<std::vector<FrameStamp>> readFrameCsv(const std::filesystem::path& path,
                                                            std::string& error) {
    std::ifstream input(path);
    if (!input) { error = "cannot_open_frames_csv"; return {}; }
    std::string line;
    if (!std::getline(input, line) || trim(line) !=
        "sequence,timestamp_ns,delta_ns,missed_intervals,write_status") {
        error = "invalid_frames_csv_header"; return {};
    }
    std::vector<FrameStamp> frames;
    while (std::getline(input, line)) {
        if (trim(line).empty()) continue;
        std::vector<std::string> f;
        if (!splitCsvLine(line, f) || f.size() != 5 || f[4] != "written") {
            error = "invalid_frames_csv_row"; return {};
        }
        FrameStamp stamp;
        if (!parseUnsigned(f[0], stamp.sequence) || !parseUnsigned(f[1], stamp.timestampNs) ||
            !parseUnsigned(f[2], stamp.deltaNs) || !parseUnsigned(f[3], stamp.missedIntervals)) {
            error = "invalid_frames_csv_number"; return {};
        }
        if (!frames.empty()) {
            const auto& prev = frames.back();
            if (stamp.sequence <= prev.sequence || stamp.timestampNs <= prev.timestampNs ||
                stamp.deltaNs == 0) { error = "non_monotonic_frame_metadata"; return {}; }
            const auto elapsed = stamp.timestampNs - prev.timestampNs;
            if (stamp.deltaNs > elapsed ||
                (stamp.sequence - prev.sequence == 1 && stamp.deltaNs != elapsed)) {
                error = "inconsistent_frame_delta"; return {};
            }
        } else if ((stamp.sequence == 0 && stamp.deltaNs != 0) ||
                   (stamp.sequence > 0 && stamp.deltaNs == 0)) {
            error = "invalid_first_frame_delta"; return {};
        }
        frames.push_back(stamp);
    }
    return frames;
}

inline std::optional<std::pair<std::uint32_t, std::uint32_t>> readPngDimensions(
    const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::array<std::uint8_t, 24> bytes{};
    if (!input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) return {};
    constexpr std::array<std::uint8_t, 8> signature{0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
    if (!std::equal(signature.begin(), signature.end(), bytes.begin()) ||
        bytes[8] != 0 || bytes[9] != 0 || bytes[10] != 0 || bytes[11] != 13 ||
        bytes[12] != 'I' || bytes[13] != 'H' || bytes[14] != 'D' || bytes[15] != 'R') return {};
    const auto read32 = [&](std::size_t i) {
        return (static_cast<std::uint32_t>(bytes[i]) << 24) |
               (static_cast<std::uint32_t>(bytes[i+1]) << 16) |
               (static_cast<std::uint32_t>(bytes[i+2]) << 8) |
               static_cast<std::uint32_t>(bytes[i+3]);
    };
    const auto width = read32(16), height = read32(20);
    if (!width || !height) return {};
    return std::pair{width, height};
}

inline std::optional<Phase> parsePhase(const std::string& value) {
    if (value == "start_idle") return Phase::StartIdle;
    if (value == "active_gameplay") return Phase::ActiveGameplay;
    if (value == "paused") return Phase::Paused;
    if (value == "respawn") return Phase::Respawn;
    if (value == "transition") return Phase::Transition;
    if (value == "unknown") return Phase::Unknown;
    return {};
}

inline std::optional<std::vector<GeometryFrame>> readGeometryCsv(
    const std::filesystem::path& path, std::string& error) {
    std::ifstream input(path);
    if (!input) { error = "cannot_open_hitboxes_csv"; return {}; }
    std::string line;
    if (!std::getline(input, line) || trim(line) !=
        "sequence,phase,geometry_status,entity_type,entity_id,left,top,right,bottom") {
        error = "invalid_hitboxes_csv_header"; return {};
    }
    std::map<std::uint64_t, GeometryFrame> frames;
    std::map<std::uint64_t, bool> metadataSeen;
    std::map<std::uint64_t, bool> sentinelSeen;
    while (std::getline(input, line)) {
        if (trim(line).empty()) continue;
        std::vector<std::string> f;
        if (!splitCsvLine(line, f) || f.size() != 9) { error = "invalid_hitboxes_csv_row"; return {}; }
        std::uint64_t sequence = 0;
        if (!parseUnsigned(f[0], sequence)) { error = "invalid_hitbox_sequence"; return {}; }
        const auto phase = parsePhase(f[1]);
        if (!phase) { error = "invalid_hitbox_phase"; return {}; }
        GeometryStatus status;
        if (f[2] == "valid") status = GeometryStatus::Valid;
        else if (f[2] == "unsupported") status = GeometryStatus::Unsupported;
        else { error = "invalid_geometry_status"; return {}; }
        auto& frame = frames[sequence];
        if (metadataSeen[sequence] && (frame.phase != *phase || frame.status != status)) {
            error = "conflicting_frame_geometry_metadata"; return {};
        }
        frame.sequence = sequence;
        frame.phase = *phase;
        frame.status = status;
        metadataSeen[sequence] = true;
        if (f[3] == "frame") {
            if (sentinelSeen[sequence] || !f[4].empty() || !f[5].empty() ||
                !f[6].empty() || !f[7].empty() || !f[8].empty()) {
                error = "invalid_frame_geometry_sentinel"; return {};
            }
            sentinelSeen[sequence] = true;
        } else if (f[3] == "player" || f[3] == "hazard") {
            std::uint64_t id = 0;
            if (!parseUnsigned(f[4], id)) { error = "invalid_geometry_entity_id"; return {}; }
            Rect rect;
            try {
                std::size_t used = 0;
                rect.left = std::stod(f[5], &used); if (used != f[5].size()) throw std::invalid_argument("left");
                rect.top = std::stod(f[6], &used); if (used != f[6].size()) throw std::invalid_argument("top");
                rect.right = std::stod(f[7], &used); if (used != f[7].size()) throw std::invalid_argument("right");
                rect.bottom = std::stod(f[8], &used); if (used != f[8].size()) throw std::invalid_argument("bottom");
            } catch (...) { error = "invalid_geometry_coordinate"; return {}; }
            if (!rect.valid()) { error = "invalid_geometry_rectangle"; return {}; }
            const auto kind = f[3] == "player" ? EntityKind::Player : EntityKind::Hazard;
            if (std::any_of(frame.entities.begin(), frame.entities.end(), [&](const auto& entity) {
                    return entity.kind == kind && entity.id == id;
                })) { error = "duplicate_geometry_entity"; return {}; }
            frame.entities.push_back({kind, id, rect});
        } else { error = "unknown_geometry_entity_type"; return {}; }
    }
    std::vector<GeometryFrame> result;
    result.reserve(frames.size());
    for (auto& [sequence, frame] : frames) {
        if (!sentinelSeen[sequence]) { error = "missing_frame_geometry_sentinel"; return {}; }
        if (frame.status == GeometryStatus::Valid &&
            std::none_of(frame.entities.begin(), frame.entities.end(),
                         [](const auto& e) { return e.kind == EntityKind::Player; })) {
            error = "valid_geometry_without_player"; return {};
        }
        result.push_back(std::move(frame));
    }
    return result;
}

inline double rectangleClearance(const Rect& a, const Rect& b) {
    const double dx = std::max({0.0, b.left - a.right, a.left - b.right});
    const double dy = std::max({0.0, b.top - a.bottom, a.top - b.bottom});
    return std::hypot(dx, dy);
}

inline ClearanceStatistics clearanceStatistics(const std::vector<double>& samples,
                                               bool unsupported = false) {
    ClearanceStatistics result;
    if (samples.empty()) {
        if (unsupported) {
            result.meanPx.status = result.medianPx.status =
                result.minimumPx.status = result.percentile10Px.status = EvidenceStatus::Unsupported;
        }
        return result;
    }
    auto sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const auto count = static_cast<std::uint32_t>(std::min<std::size_t>(
        sorted.size(), std::numeric_limits<std::uint32_t>::max()));
    const double mean = std::accumulate(sorted.begin(), sorted.end(), 0.0) /
                        static_cast<double>(sorted.size());
    const auto quantile = [&](double q) {
        const auto index = static_cast<std::size_t>(std::ceil(q * sorted.size()) - 1.0);
        return sorted[std::min(index, sorted.size() - 1)];
    };
    result.meanPx = {mean, count, EvidenceStatus::Measured};
    result.medianPx = {quantile(0.5), count, EvidenceStatus::Measured};
    result.minimumPx = {sorted.front(), count, EvidenceStatus::Measured};
    result.percentile10Px = {quantile(0.1), count, EvidenceStatus::Measured};
    return result;
}

inline std::optional<ParseResult> parseSession(const std::filesystem::path& session,
                                               std::string& error) {
    const auto manifest = readManifest(session / "manifest.json", error);
    if (!manifest) return {};
    const auto frames = readFrameCsv(session / manifest->framesMetadata, error);
    if (!frames) return {};
    if (frames->size() != manifest->framesWritten) { error = "frames_written_count_mismatch"; return {}; }
    if (frames->empty() != !manifest->firstSequence ||
        (!frames->empty() && (frames->front().sequence != *manifest->firstSequence ||
                              frames->back().sequence != *manifest->lastSequence))) {
        error = "manifest_frame_sequence_mismatch"; return {};
    }
    for (const auto& stamp : *frames) {
        std::ostringstream filename;
        filename << "frame_" << std::setw(6) << std::setfill('0') << stamp.sequence << ".png";
        const auto dimensions = readPngDimensions(session / manifest->framesDirectory / filename.str());
        if (!dimensions || dimensions->first != manifest->width || dimensions->second != manifest->height) {
            error = "missing_or_wrong_sized_png"; return {};
        }
    }
    const auto geometry = readGeometryCsv(session / "hitboxes.csv", error);
    if (!geometry) return {};
    if (geometry->size() != frames->size()) { error = "geometry_frame_count_mismatch"; return {}; }
    std::map<std::uint64_t, const GeometryFrame*> bySequence;
    for (const auto& frame : *geometry) bySequence.emplace(frame.sequence, &frame);
    for (const auto& stamp : *frames)
        if (!bySequence.contains(stamp.sequence)) { error = "missing_frame_geometry"; return {}; }
    for (const auto& frame : *geometry)
        if (std::none_of(frames->begin(), frames->end(), [&](const auto& s) {
                return s.sequence == frame.sequence;
            })) { error = "geometry_for_unknown_frame"; return {}; }

    ParseResult result;
    result.structurallyValid = true;
    result.sourceFrameCount = frames->size();
    result.missedCaptureIntervals = manifest->missedCaptureIntervals;
    result.writerQueueDrops = manifest->writerQueueDrops;
    const double clearanceScale = kReferenceCaptureWidth / static_cast<double>(manifest->width);
    std::vector<double> allClearances;
    for (std::size_t begin = 0; begin < frames->size();) {
        const auto phase = bySequence.at((*frames)[begin].sequence)->phase;
        std::size_t end = begin;
        while (end < frames->size() && end - begin < kImagesPerSection &&
               bySequence.at((*frames)[end].sequence)->phase == phase) ++end;
        Section section;
        section.index = static_cast<std::uint32_t>(result.sections.size());
        section.firstSampleIndex = begin;
        section.lastSampleIndex = end - 1;
        section.phase = phase;
        std::vector<double> sectionClearances;
        for (std::size_t i = begin; i < end; ++i) {
            const auto& stamp = (*frames)[i];
            const auto& geometryFrame = *bySequence.at(stamp.sequence);
            const bool gap = (i == 0 && (stamp.sequence > 0 || stamp.missedIntervals > 0)) ||
                (i > 0 && (stamp.sequence != (*frames)[i-1].sequence + 1 || stamp.missedIntervals > 0));
            if (gap) ++result.timestampGapCount;
            if (geometryFrame.status == GeometryStatus::Unsupported) {
                ++section.unsupportedHitboxCount;
                ++result.unclassifiedFrameCount;
                continue;
            }
            if (phase != Phase::ActiveGameplay) continue;
            std::vector<const Rect*> players, hazards;
            for (const auto& entity : geometryFrame.entities) {
                if (entity.kind == EntityKind::Player) players.push_back(&entity.bounds);
                if (entity.kind == EntityKind::Hazard) hazards.push_back(&entity.bounds);
            }
            // No hazard rows means no clearance sample, not a zero/safe sample.
            if (players.empty() || hazards.empty()) continue;
            double closest = std::numeric_limits<double>::infinity();
            for (const auto* player : players)
                for (const auto* hazard : hazards)
                    closest = std::min(closest, rectangleClearance(*player, *hazard) * clearanceScale);
            if (std::isfinite(closest)) {
                sectionClearances.push_back(closest);
                allClearances.push_back(closest);
            }
        }
        section.clearance = clearanceStatistics(sectionClearances, section.unsupportedHitboxCount > 0);
        // Timing windows, Nerves and skillsets stay Missing until their telemetry
        // and calculation contracts are defined and supplied by the Recorder.
        result.sections.push_back(std::move(section));
        begin = end;
    }
    result.processedFrameCount = result.sourceFrameCount - result.unclassifiedFrameCount;
    result.clearance = clearanceStatistics(allClearances, result.unclassifiedFrameCount > 0);
    result.parserComplete = result.unclassifiedFrameCount == 0 && !result.sections.empty();
    error.clear();
    return result;
}

} // namespace geodan::core::image_parser
