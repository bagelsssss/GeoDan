#pragma once

// Geode-free evidence model for GeoDan's Dan Analyzer.
// This layer records measurements and provenance; it deliberately does not
// simulate player physics, classify screenshots, or assign dan scores.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace geodan::core::dan {

inline constexpr std::uint32_t kEvidenceSchemaVersion = 1;

enum class GameMode : std::uint8_t { Unknown, Classic, Platformer };
enum class EvidenceState : std::uint8_t { Unknown, Observed, Measured, Validated, Unsupported };
enum class EvidenceSource : std::uint8_t { Unknown, Geometry, Simulation, Visual, GameplayTelemetry };

enum class Skillset : std::uint8_t {
    Circles, Clicksync, FastPaced, Timings, Chokepoints, Unbalanced, Learny, Memory,
    HighCps, Gimmicky, Flow, SlowPaced, Bossfight, Mirror, NerveControl, Cube, Ship,
    Ball, Ufo, Wave, Robot, Spider, OldSwing, NewSwing, Duals, Overall,
};
inline constexpr std::size_t kSkillsetCount = 26;

enum class DanTier : std::uint8_t {
    First = 1, Second, Third, Fourth, Fifth, Sixth, Seventh, Eighth, Ninth, Tenth,
};

// A measurement is meaningful only together with its source, evidence state,
// and sample count. present=false means unknown, not zero.
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

    bool validatedFrom(EvidenceSource expectedSource) const {
        return present && valid() && state == EvidenceState::Validated &&
               source == expectedSource;
    }
};

struct SegmentEvidence {
    std::uint32_t index = 0;
    // Inclusive sample-index range, independent of world orientation or mode.
    std::uint64_t firstSampleIndex = 0;
    std::uint64_t lastSampleIndex = 0;
    GameMode mode = GameMode::Unknown;

    Measurement minimumClearance;
    Measurement hazardDensityPer1000Units;
    Measurement tightestTimingWindowFrames;
    Measurement requiredInputsPerSecond;
    Measurement peakInputsPerSecond;
    Measurement successfulTrialRate;
    Measurement attemptsToComplete;
    std::uint32_t unsupportedInteractions = 0;

    bool valid() const {
        if (mode == GameMode::Unknown || lastSampleIndex < firstSampleIndex)
            return false;
        const auto nonNegative = [](const Measurement& m) {
            return !m.present || m.value >= 0.0;
        };
        return minimumClearance.valid() && hazardDensityPer1000Units.valid() &&
               tightestTimingWindowFrames.valid() && requiredInputsPerSecond.valid() &&
               peakInputsPerSecond.valid() && successfulTrialRate.valid() &&
               attemptsToComplete.valid() && nonNegative(hazardDensityPer1000Units) &&
               nonNegative(tightestTimingWindowFrames) &&
               nonNegative(requiredInputsPerSecond) && nonNegative(peakInputsPerSecond) &&
               nonNegative(attemptsToComplete) &&
               (!successfulTrialRate.present ||
                (successfulTrialRate.value >= 0.0 && successfulTrialRate.value <= 1.0));
    }

    bool hasValidatedGeometryEvidence() const {
        return minimumClearance.validatedFrom(EvidenceSource::Geometry) ||
               hazardDensityPer1000Units.validatedFrom(EvidenceSource::Geometry);
    }

    bool hasValidatedSimulationEvidence() const {
        return tightestTimingWindowFrames.validatedFrom(EvidenceSource::Simulation) ||
               requiredInputsPerSecond.validatedFrom(EvidenceSource::Simulation) ||
               peakInputsPerSecond.validatedFrom(EvidenceSource::Simulation) ||
               successfulTrialRate.validatedFrom(EvidenceSource::Simulation) ||
               attemptsToComplete.validatedFrom(EvidenceSource::Simulation);
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

// Keep both memory signals separate. The exact "both" rule and numeric
// threshold are not guessed here; see the design document.
struct MemoryEvidence {
    std::uint32_t visualObstructionCount = 0;
    std::uint32_t simulationUnclearPathCount = 0;
};

struct LevelEvidence {
    std::uint32_t schemaVersion = kEvidenceSchemaVersion;
    std::string levelName;
    GameMode primaryMode = GameMode::Unknown;
    bool captureComplete = false;
    bool simulationComplete = false;
    std::vector<SegmentEvidence> segments;
    std::vector<SkillsetEvidence> skillsets;
    MemoryEvidence memory;
};

enum class AnalysisReadiness : std::uint8_t {
    NotReady, InvalidEvidence, Provisional, Measured, CalibrationEligible,
};

inline AnalysisReadiness assessReadiness(const LevelEvidence& level) {
    if (level.schemaVersion != kEvidenceSchemaVersion ||
        !level.captureComplete || !level.simulationComplete || level.segments.empty())
        return AnalysisReadiness::NotReady;

    bool anyValidated = false;
    bool allSegmentsHaveBothSources = true;

    for (const auto& segment : level.segments) {
        if (!segment.valid()) return AnalysisReadiness::InvalidEvidence;
        anyValidated = anyValidated || segment.hasValidatedGeometryEvidence() ||
                       segment.hasValidatedSimulationEvidence();
        allSegmentsHaveBothSources = allSegmentsHaveBothSources &&
            segment.unsupportedInteractions == 0 &&
            segment.hasValidatedGeometryEvidence() &&
            segment.hasValidatedSimulationEvidence();
    }

    for (const auto& skillset : level.skillsets) {
        if (!skillset.valid()) return AnalysisReadiness::InvalidEvidence;
    }

    if (allSegmentsHaveBothSources) return AnalysisReadiness::CalibrationEligible;
    if (anyValidated) return AnalysisReadiness::Measured;
    return AnalysisReadiness::Provisional;
}

inline bool canAssignDan(const LevelEvidence& level) {
    return assessReadiness(level) == AnalysisReadiness::CalibrationEligible;
}

} // namespace geodan::core::dan
