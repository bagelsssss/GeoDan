#pragma once

// Portable, Geode-free judge model for GeoDan.
// The Image Parser supplies recorded-run measurements. Swag-route search
// supplies separately validated alternative-route evidence. This model does
// not recreate the recorded run or assign a calibrated Dan tier.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace geodan::core::dan {

inline constexpr std::uint32_t kEvidenceSchemaVersion = 2;

enum class EvidenceState : std::uint8_t { Unknown, Observed, Measured, Validated, Unsupported };
enum class EvidenceSource : std::uint8_t { Unknown, HitboxGeometry, ImageParser, SwagRouteSearch, GameplayTelemetry };
enum class SegmentPhase : std::uint8_t { Unknown, StartIdle, ActiveGameplay, Paused, Respawn, Transition };
enum class Skillset : std::uint8_t {
    Circles, Clicksync, FastPaced, Timings, Chokepoints, Unbalanced, Learny,
    Memory, HighCps, Gimmicky, Flow, SlowPaced, Bossfight, Mirror,
    NerveControl, Cube, Ship, Ball, Ufo, Wave, Robot, Spider,
    OldSwing, NewSwing, Duals, Overall,
};
inline constexpr std::size_t kSkillsetCount = 26;

enum class DanTier : std::uint8_t {
    First = 1, Second, Third, Fourth, Fifth, Sixth, Seventh, Eighth, Ninth, Tenth,
};

struct Measurement {
    double value = 0.0;
    std::uint32_t sampleCount = 0;
    EvidenceState state = EvidenceState::Unknown;
    EvidenceSource source = EvidenceSource::Unknown;
    bool present = false;

    bool valid() const {
        if (!present) return true;
        return std::isfinite(value) && sampleCount > 0 &&
               state != EvidenceState::Unknown && source != EvidenceSource::Unknown;
    }

    bool validatedFrom(EvidenceSource expected) const {
        return present && valid() && state == EvidenceState::Validated && source == expected;
    }
};

struct FrameWindowCounts {
    Measurement windows60;
    Measurement windows120;
    Measurement windows240;
};

struct ClearanceStatistics {
    Measurement meanPx;
    Measurement medianPx;
    Measurement minimumPx;
    Measurement percentile10Px;
};

struct SegmentEvidence {
    std::uint32_t index = 0;
    std::uint64_t firstSampleIndex = 0;
    std::uint64_t lastSampleIndex = 0;
    SegmentPhase phase = SegmentPhase::Unknown;
    ClearanceStatistics clearance;
    FrameWindowCounts frameWindows;
    Measurement nerves;
    std::uint32_t unsupportedHitboxCount = 0;

    bool valid() const {
        if (phase == SegmentPhase::Unknown || lastSampleIndex < firstSampleIndex) return false;
        const Measurement* values[] = {
            &clearance.meanPx, &clearance.medianPx, &clearance.minimumPx,
            &clearance.percentile10Px, &frameWindows.windows60,
            &frameWindows.windows120, &frameWindows.windows240, &nerves,
        };
        for (const auto* value : values) {
            if (!value->valid()) return false;
            if (value->present && value->value < 0.0) return false;
        }
        if (clearance.minimumPx.present && clearance.medianPx.present &&
            clearance.minimumPx.value > clearance.medianPx.value) return false;
        if (clearance.minimumPx.present && clearance.meanPx.present &&
            clearance.minimumPx.value > clearance.meanPx.value) return false;
        if (clearance.percentile10Px.present && clearance.medianPx.present &&
            clearance.percentile10Px.value > clearance.medianPx.value) return false;
        return true;
    }
};

struct SkillsetEvidence {
    Skillset skillset = Skillset::Overall;
    std::uint32_t positiveObservations = 0;
    std::uint32_t totalObservations = 0;
    EvidenceState state = EvidenceState::Unknown;

    bool valid() const {
        return positiveObservations <= totalObservations &&
               (totalObservations == 0 || state != EvidenceState::Unknown);
    }
};

struct MemoryEvidence {
    Measurement visualObstructionCount;
    Measurement simulationUnclearPathCount;
};

enum class RouteSearchState : std::uint8_t { NotRun, Complete, Partial, Unsupported };
enum class RouteKind : std::uint8_t { Reference, Alternative };
enum class RouteOutcome : std::uint8_t { Unknown, Completed, Failed, Unsupported };
enum class DifficultyRelation : std::uint8_t { Unknown, Easier, Equal, Harder };

struct SwagRouteEvidence {
    std::uint32_t routeId = 0;
    RouteKind kind = RouteKind::Alternative;
    RouteOutcome outcome = RouteOutcome::Unknown;
    DifficultyRelation relationToReference = DifficultyRelation::Unknown;
    // Fraction of the reference route's difficulty-bearing gameplay bypassed.
    // This is not geometric distance or elapsed-time fraction.
    Measurement bypassedDifficultyFraction;

    bool valid() const {
        if (kind == RouteKind::Reference && relationToReference != DifficultyRelation::Equal)
            return false;
        if (!bypassedDifficultyFraction.valid()) return false;
        if (bypassedDifficultyFraction.present &&
            (bypassedDifficultyFraction.value < 0.0 || bypassedDifficultyFraction.value > 1.0))
            return false;
        if (outcome == RouteOutcome::Completed && relationToReference == DifficultyRelation::Unknown)
            return false;
        return true;
    }

    bool affectsDifficulty() const {
        return kind == RouteKind::Alternative && outcome == RouteOutcome::Completed &&
               relationToReference == DifficultyRelation::Easier &&
               bypassedDifficultyFraction.validatedFrom(EvidenceSource::SwagRouteSearch);
    }
};

struct LevelParserSummary {
    std::uint64_t sourceFrameCount = 0;
    std::uint64_t processedFrameCount = 0;
    std::uint64_t unclassifiedFrameCount = 0;
    std::uint64_t timestampGapCount = 0;
    ClearanceStatistics clearance;
    FrameWindowCounts frameWindows;
    Measurement nerves;
};

struct LevelEvidence {
    std::uint32_t schemaVersion = kEvidenceSchemaVersion;
    std::string levelName;
    bool captureComplete = false;
    bool parserComplete = false;
    LevelParserSummary parser;
    std::vector<SegmentEvidence> segments;
    std::vector<SkillsetEvidence> skillsets;
    MemoryEvidence memory;
    RouteSearchState routeSearch = RouteSearchState::NotRun;
    std::vector<SwagRouteEvidence> routes;
};

enum class JudgeStatus : std::uint8_t {
    Rejected, InsufficientEvidence, ValidatedForCalibration,
};

struct JudgeResult {
    JudgeStatus status = JudgeStatus::InsufficientEvidence;
    std::uint32_t activeSegmentCount = 0;
    std::uint32_t validatedClearanceSegmentCount = 0;
    std::uint32_t validatedTimingSegmentCount = 0;
    std::uint32_t validatedNervesSegmentCount = 0;
    std::uint32_t confirmedEasierRouteCount = 0;
    std::uint32_t unsupportedEvidenceCount = 0;
};

inline bool validClearanceStatistics(const ClearanceStatistics& stats) {
    const Measurement* values[] = {&stats.meanPx, &stats.medianPx,
                                   &stats.minimumPx, &stats.percentile10Px};
    for (const auto* value : values) {
        if (!value->valid() || (value->present && value->value < 0.0)) return false;
    }
    if (stats.minimumPx.present && stats.medianPx.present &&
        stats.minimumPx.value > stats.medianPx.value) return false;
    if (stats.minimumPx.present && stats.meanPx.present &&
        stats.minimumPx.value > stats.meanPx.value) return false;
    if (stats.percentile10Px.present && stats.medianPx.present &&
        stats.percentile10Px.value > stats.medianPx.value) return false;
    return true;
}

inline bool validFrameWindows(const FrameWindowCounts& counts) {
    const Measurement* values[] = {&counts.windows60, &counts.windows120, &counts.windows240};
    for (const auto* value : values) {
        if (!value->valid() || (value->present && value->value < 0.0)) return false;
    }
    return true;
}

inline JudgeResult judge(const LevelEvidence& level) {
    JudgeResult result;
    if (level.schemaVersion != kEvidenceSchemaVersion || !level.captureComplete ||
        !level.parserComplete || level.segments.empty() ||
        level.parser.sourceFrameCount == 0 ||
        level.parser.processedFrameCount + level.parser.unclassifiedFrameCount !=
            level.parser.sourceFrameCount ||
        !validClearanceStatistics(level.parser.clearance) ||
        !validFrameWindows(level.parser.frameWindows) || !level.parser.nerves.valid()) {
        result.status = JudgeStatus::Rejected;
        return result;
    }

    bool allActiveSegmentsValidated = true;
    for (const auto& segment : level.segments) {
        if (!segment.valid()) {
            result.status = JudgeStatus::Rejected;
            return result;
        }
        if (segment.phase != SegmentPhase::ActiveGameplay) continue;
        ++result.activeSegmentCount;
        const bool clearanceOk = segment.clearance.meanPx.validatedFrom(EvidenceSource::ImageParser) &&
                                 segment.clearance.medianPx.validatedFrom(EvidenceSource::ImageParser) &&
                                 segment.clearance.minimumPx.validatedFrom(EvidenceSource::ImageParser);
        const bool timingOk = segment.frameWindows.windows60.validatedFrom(EvidenceSource::ImageParser) &&
                              segment.frameWindows.windows120.validatedFrom(EvidenceSource::ImageParser) &&
                              segment.frameWindows.windows240.validatedFrom(EvidenceSource::ImageParser);
        const bool nervesOk = segment.nerves.validatedFrom(EvidenceSource::ImageParser);
        if (clearanceOk) ++result.validatedClearanceSegmentCount;
        if (timingOk) ++result.validatedTimingSegmentCount;
        if (nervesOk) ++result.validatedNervesSegmentCount;
        allActiveSegmentsValidated = allActiveSegmentsValidated && clearanceOk && timingOk && nervesOk;
        result.unsupportedEvidenceCount += segment.unsupportedHitboxCount;
    }
    if (result.activeSegmentCount == 0) {
        result.status = JudgeStatus::Rejected;
        return result;
    }

    for (const auto& skillset : level.skillsets) {
        if (!skillset.valid()) {
            result.status = JudgeStatus::Rejected;
            return result;
        }
    }
    if (!level.memory.visualObstructionCount.valid() ||
        !level.memory.simulationUnclearPathCount.valid()) {
        result.status = JudgeStatus::Rejected;
        return result;
    }

    bool referenceFound = false;
    for (const auto& route : level.routes) {
        if (!route.valid()) {
            result.status = JudgeStatus::Rejected;
            return result;
        }
        if (route.kind == RouteKind::Reference && route.outcome == RouteOutcome::Completed)
            referenceFound = true;
        if (route.affectsDifficulty()) ++result.confirmedEasierRouteCount;
        if (route.outcome == RouteOutcome::Unsupported) ++result.unsupportedEvidenceCount;
    }

    const bool routeEvidenceReady = level.routeSearch == RouteSearchState::Complete && referenceFound;
    if (allActiveSegmentsValidated && result.unsupportedEvidenceCount == 0 && routeEvidenceReady) {
        result.status = JudgeStatus::ValidatedForCalibration;
    } else {
        result.status = JudgeStatus::InsufficientEvidence;
    }
    return result;
}

} // namespace geodan::core::dan
