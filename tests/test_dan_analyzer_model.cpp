#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

#include "dan_analyzer_model.hpp"

namespace dan = geodan::core::dan;

static dan::Measurement metric(double value, dan::EvidenceSource source,
                               dan::EvidenceState state = dan::EvidenceState::Validated) {
    return {value, 12, state, source, true};
}

int main() {
    static_assert(dan::kSkillsetCount == 26);
    static_assert(static_cast<unsigned>(dan::DanTier::Tenth) == 10);

    dan::LevelEvidence level;
    level.levelName = "synthetic";
    assert(dan::assessReadiness(level) == dan::AnalysisReadiness::NotReady);
    assert(!dan::canAssignDan(level));

    dan::SegmentEvidence segment;
    segment.index = 0;
    segment.firstSampleIndex = 0;
    segment.lastSampleIndex = 300;
    segment.mode = dan::GameMode::Classic;
    segment.minimumClearance = metric(2.5, dan::EvidenceSource::Geometry);
    segment.tightestTimingWindowFrames = metric(3.0, dan::EvidenceSource::Simulation);
    level.segments.push_back(segment);
    level.captureComplete = true;
    level.simulationComplete = true;

    assert(dan::assessReadiness(level) == dan::AnalysisReadiness::CalibrationEligible);
    assert(dan::canAssignDan(level));

    level.segments[0].minimumClearance = {};
    assert(dan::assessReadiness(level) == dan::AnalysisReadiness::Measured);
    assert(!dan::canAssignDan(level));

    level.segments[0].minimumClearance =
        metric(0.0, dan::EvidenceSource::Geometry, dan::EvidenceState::Observed);
    assert(dan::assessReadiness(level) == dan::AnalysisReadiness::Measured);

    level.segments[0].minimumClearance =
        metric(std::numeric_limits<double>::quiet_NaN(), dan::EvidenceSource::Geometry);
    assert(dan::assessReadiness(level) == dan::AnalysisReadiness::InvalidEvidence);

    level.segments[0].minimumClearance = metric(2.5, dan::EvidenceSource::Geometry);
    level.segments[0].hazardDensityPer1000Units =
        metric(0.0, dan::EvidenceSource::Geometry, dan::EvidenceState::Unsupported);
    assert(dan::assessReadiness(level) == dan::AnalysisReadiness::CalibrationEligible);
    level.segments[0].hazardDensityPer1000Units = {};
    level.segments[0].unsupportedInteractions = 1;
    assert(dan::assessReadiness(level) == dan::AnalysisReadiness::Measured);
    assert(!dan::canAssignDan(level));

    level.segments[0].unsupportedInteractions = 0;
    level.segments[0].successfulTrialRate =
        metric(1.5, dan::EvidenceSource::Simulation);
    assert(dan::assessReadiness(level) == dan::AnalysisReadiness::InvalidEvidence);
    level.segments[0].successfulTrialRate = {};
    level.skillsets.push_back({dan::Skillset::Wave, 5, 4, dan::EvidenceState::Measured});
    assert(dan::assessReadiness(level) == dan::AnalysisReadiness::InvalidEvidence);

    std::cout << "Dan analyzer evidence model: all checks passed\n";
    return 0;
}
