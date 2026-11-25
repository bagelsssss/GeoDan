#pragma once

// Portable, Geode-free judge model for GeoDan.
// The Image Parser supplies recorded-run measurements. Swag-route search
// supplies separately validated alternative-route evidence. This model does
// not recreate the recorded run or assign a calibrated Dan tier.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
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

struct DistributionAggregate {
    Measurement weightedMean;
    Measurement medianOfSections;
    Measurement percentile10OfSections;
    Measurement minimumOfSections;
    Measurement maximumOfSections;
    std::uint32_t sectionCount = 0;
    std::uint64_t representedSamples = 0;
};

struct LevelFeatureAggregate {
    DistributionAggregate sectionMeanClearance;
    DistributionAggregate sectionMedianClearance;
    DistributionAggregate sectionMinimumClearance;
    DistributionAggregate nervesBySection;
    std::uint64_t totalWindows60 = 0;
    std::uint64_t totalWindows120 = 0;
    std::uint64_t totalWindows240 = 0;
    std::uint32_t timingSections60 = 0;
    std::uint32_t timingSections120 = 0;
    std::uint32_t timingSections240 = 0;
    std::uint32_t activeSectionCount = 0;
    std::uint32_t confirmedEasierRouteCount = 0;
    Measurement maximumDifficultyBypassFraction;
};

enum class ConsistencyStatus : std::uint8_t { Consistent, Incomplete, Inconsistent };

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
    ConsistencyStatus consistency = ConsistencyStatus::Incomplete;
    LevelFeatureAggregate aggregate;
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

inline bool approximatelyEqual(double a, double b) {
    const double scale = std::max({1.0, std::abs(a), std::abs(b)});
    return std::abs(a - b) <= scale * 1e-6;
}

inline Measurement aggregateMeasurement(const std::vector<std::pair<double, std::uint32_t>>& values,
                                        double quantile, bool weightedMean) {
    if (values.empty()) return {};
    std::vector<std::pair<double, std::uint32_t>> sorted = values;
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    std::uint64_t totalWeight = 0;
    long double weightedSum = 0.0L;
    for (const auto& item : sorted) {
        totalWeight += item.second;
        weightedSum += static_cast<long double>(item.first) * item.second;
    }
    if (totalWeight == 0) return {};
    double value = 0.0;
    if (weightedMean) {
        value = static_cast<double>(weightedSum / totalWeight);
    } else {
        const auto target = std::max<std::uint64_t>(
            1, static_cast<std::uint64_t>(std::ceil(quantile * totalWeight)));
        std::uint64_t cumulative = 0;
        for (const auto& item : sorted) {
            cumulative += item.second;
            if (cumulative >= target) {
                value = item.first;
                break;
            }
        }
    }
    const auto samples = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(totalWeight, std::numeric_limits<std::uint32_t>::max()));
    return {value, samples, EvidenceState::Measured, EvidenceSource::ImageParser, true};
}

inline DistributionAggregate aggregateSectionMetric(
    const std::vector<const Measurement*>& measurements) {
    DistributionAggregate result;
    std::vector<std::pair<double, std::uint32_t>> values;
    for (const auto* measurement : measurements) {
        if (!measurement || !measurement->present || !measurement->valid() ||
            measurement->state == EvidenceState::Unsupported ||
            measurement->source != EvidenceSource::ImageParser) continue;
        values.emplace_back(measurement->value, measurement->sampleCount);
        result.representedSamples += measurement->sampleCount;
    }
    if (values.empty()) return result;
    result.sectionCount = static_cast<std::uint32_t>(values.size());
    result.weightedMean = aggregateMeasurement(values, 0.0, true);
    result.medianOfSections = aggregateMeasurement(values, 0.5, false);
    result.percentile10OfSections = aggregateMeasurement(values, 0.1, false);
    const auto [minimum, maximum] = std::minmax_element(
        values.begin(), values.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    result.minimumOfSections = {minimum->first, minimum->second,
        EvidenceState::Measured, EvidenceSource::ImageParser, true};
    result.maximumOfSections = {maximum->first, maximum->second,
        EvidenceState::Measured, EvidenceSource::ImageParser, true};
    return result;
}

inline LevelFeatureAggregate aggregateActiveSections(const LevelEvidence& level) {
    LevelFeatureAggregate result;
    std::vector<const Measurement*> means, medians, minimums, nerves;
    for (const auto& segment : level.segments) {
        if (segment.phase != SegmentPhase::ActiveGameplay) continue;
        ++result.activeSectionCount;
        means.push_back(&segment.clearance.meanPx);
        medians.push_back(&segment.clearance.medianPx);
        minimums.push_back(&segment.clearance.minimumPx);
        nerves.push_back(&segment.nerves);
        if (segment.frameWindows.windows60.present &&
            segment.frameWindows.windows60.valid() &&
            segment.frameWindows.windows60.state != EvidenceState::Unsupported) {
            result.totalWindows60 += static_cast<std::uint64_t>(segment.frameWindows.windows60.value);
            ++result.timingSections60;
        }
        if (segment.frameWindows.windows120.present &&
            segment.frameWindows.windows120.valid() &&
            segment.frameWindows.windows120.state != EvidenceState::Unsupported) {
            result.totalWindows120 += static_cast<std::uint64_t>(segment.frameWindows.windows120.value);
            ++result.timingSections120;
        }
        if (segment.frameWindows.windows240.present &&
            segment.frameWindows.windows240.valid() &&
            segment.frameWindows.windows240.state != EvidenceState::Unsupported) {
            result.totalWindows240 += static_cast<std::uint64_t>(segment.frameWindows.windows240.value);
            ++result.timingSections240;
        }
    }
    result.sectionMeanClearance = aggregateSectionMetric(means);
    result.sectionMedianClearance = aggregateSectionMetric(medians);
    result.sectionMinimumClearance = aggregateSectionMetric(minimums);
    result.nervesBySection = aggregateSectionMetric(nerves);

    for (const auto& route : level.routes) {
        if (!route.affectsDifficulty()) continue;
        ++result.confirmedEasierRouteCount;
        if (!result.maximumDifficultyBypassFraction.present ||
            route.bypassedDifficultyFraction.value >
                result.maximumDifficultyBypassFraction.value) {
            result.maximumDifficultyBypassFraction = route.bypassedDifficultyFraction;
        }
    }
    return result;
}

inline ConsistencyStatus validateAggregateConsistency(
    const LevelEvidence& level, const LevelFeatureAggregate& aggregate) {
    bool complete = aggregate.activeSectionCount > 0 &&
        aggregate.sectionMeanClearance.sectionCount == aggregate.activeSectionCount &&
        aggregate.sectionMinimumClearance.sectionCount == aggregate.activeSectionCount &&
        aggregate.nervesBySection.sectionCount == aggregate.activeSectionCount &&
        aggregate.timingSections60 == aggregate.activeSectionCount &&
        aggregate.timingSections120 == aggregate.activeSectionCount &&
        aggregate.timingSections240 == aggregate.activeSectionCount;
    if (!complete) return ConsistencyStatus::Incomplete;

    const auto& summary = level.parser;
    const auto& mean = summary.clearance.meanPx;
    const auto& minimum = summary.clearance.minimumPx;
    if (!mean.present || !minimum.present ||
        !summary.frameWindows.windows60.present ||
        !summary.frameWindows.windows120.present ||
        !summary.frameWindows.windows240.present ||
        !summary.nerves.present) return ConsistencyStatus::Incomplete;

    if (!approximatelyEqual(mean.value, aggregate.sectionMeanClearance.weightedMean.value) ||
        !approximatelyEqual(minimum.value, aggregate.sectionMinimumClearance.minimumOfSections.value) ||
        !approximatelyEqual(summary.frameWindows.windows60.value,
                            static_cast<double>(aggregate.totalWindows60)) ||
        !approximatelyEqual(summary.frameWindows.windows120.value,
                            static_cast<double>(aggregate.totalWindows120)) ||
        !approximatelyEqual(summary.frameWindows.windows240.value,
                            static_cast<double>(aggregate.totalWindows240)) ||
        !approximatelyEqual(summary.nerves.value, aggregate.nervesBySection.weightedMean.value)) {
        return ConsistencyStatus::Inconsistent;
    }
    if (mean.sampleCount != aggregate.sectionMeanClearance.weightedMean.sampleCount)
        return ConsistencyStatus::Inconsistent;
    return ConsistencyStatus::Consistent;
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

    result.aggregate = aggregateActiveSections(level);
    result.consistency = validateAggregateConsistency(level, result.aggregate);
    result.confirmedEasierRouteCount = result.aggregate.confirmedEasierRouteCount;

    if (result.consistency == ConsistencyStatus::Inconsistent) {
        result.status = JudgeStatus::Rejected;
        return result;
    }

    const bool routeEvidenceReady = level.routeSearch == RouteSearchState::Complete && referenceFound;
    if (allActiveSegmentsValidated && result.unsupportedEvidenceCount == 0 &&
        routeEvidenceReady && result.consistency == ConsistencyStatus::Consistent) {
        result.status = JudgeStatus::ValidatedForCalibration;
    } else {
        result.status = JudgeStatus::InsufficientEvidence;
    }
    return result;
}

} // namespace geodan::core::dan
