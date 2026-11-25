#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include "dan_analyzer_model.hpp"

namespace dan = geodan::core::dan;

static dan::Measurement m(double value, dan::EvidenceSource source,
                          dan::EvidenceState state = dan::EvidenceState::Validated,
                          std::uint32_t samples = 10) {
    return {value, samples, state, source, true};
}

static dan::LevelEvidence validLevel() {
    dan::LevelEvidence level;
    level.levelName = "synthetic classic level";
    level.captureComplete = true;
    level.parserComplete = true;
    level.parser.sourceFrameCount = 20;
    level.parser.processedFrameCount = 20;
    level.parser.clearance = {m(8, dan::EvidenceSource::ImageParser),
                              m(7, dan::EvidenceSource::ImageParser),
                              m(2, dan::EvidenceSource::ImageParser),
                              m(4, dan::EvidenceSource::ImageParser)};
    level.parser.frameWindows = {m(2, dan::EvidenceSource::ImageParser),
                                 m(1, dan::EvidenceSource::ImageParser),
                                 m(0, dan::EvidenceSource::ImageParser)};
    level.parser.nerves = m(0.6, dan::EvidenceSource::ImageParser);

    dan::SegmentEvidence segment;
    segment.index = 0;
    segment.firstSampleIndex = 0;
    segment.lastSampleIndex = 19;
    segment.phase = dan::SegmentPhase::ActiveGameplay;
    segment.clearance = level.parser.clearance;
    segment.frameWindows = level.parser.frameWindows;
    segment.nerves = level.parser.nerves;
    level.segments.push_back(segment);

    level.routeSearch = dan::RouteSearchState::Complete;
    dan::SwagRouteEvidence reference;
    reference.routeId = 0;
    reference.kind = dan::RouteKind::Reference;
    reference.outcome = dan::RouteOutcome::Completed;
    reference.relationToReference = dan::DifficultyRelation::Equal;
    level.routes.push_back(reference);
    return level;
}

static dan::SegmentEvidence makeSegment(std::uint32_t index, std::uint64_t first,
                                        std::uint64_t last, double mean, double median,
                                        double minimum, double p10, double nerves,
                                        double w60, double w120, double w240) {
    dan::SegmentEvidence segment;
    segment.index = index;
    segment.firstSampleIndex = first;
    segment.lastSampleIndex = last;
    segment.phase = dan::SegmentPhase::ActiveGameplay;
    segment.clearance = {m(mean, dan::EvidenceSource::ImageParser),
                         m(median, dan::EvidenceSource::ImageParser),
                         m(minimum, dan::EvidenceSource::ImageParser),
                         m(p10, dan::EvidenceSource::ImageParser)};
    segment.frameWindows = {m(w60, dan::EvidenceSource::ImageParser),
                            m(w120, dan::EvidenceSource::ImageParser),
                            m(w240, dan::EvidenceSource::ImageParser)};
    segment.nerves = m(nerves, dan::EvidenceSource::ImageParser);
    return segment;
}

int main() {
    static_assert(dan::kSkillsetCount == 26);
    static_assert(static_cast<unsigned>(dan::DanTier::Tenth) == 10);

    auto level = validLevel();
    auto judged = dan::judge(level);
    assert(judged.status == dan::JudgeStatus::ValidatedForCalibration);
    assert(judged.consistency == dan::ConsistencyStatus::Consistent);
    assert(judged.aggregate.activeSectionCount == 1);
    assert(judged.aggregate.sectionMeanClearance.weightedMean.value == 8.0);
    assert(judged.aggregate.totalWindows60 == 2);
    assert(judged.aggregate.totalWindows120 == 1);
    assert(judged.aggregate.totalWindows240 == 0);
    assert(judged.aggregate.nervesBySection.weightedMean.value == 0.6);

    // Startup idle is retained but excluded from active difficulty aggregates.
    auto idle = level.segments.front();
    idle.index = 1;
    idle.firstSampleIndex = 20;
    idle.lastSampleIndex = 25;
    idle.phase = dan::SegmentPhase::StartIdle;
    idle.clearance = {};
    idle.frameWindows = {};
    idle.nerves = {};
    level.parser.sourceFrameCount = 26;
    level.parser.processedFrameCount = 26;
    level.segments.push_back(idle);
    judged = dan::judge(level);
    assert(judged.status == dan::JudgeStatus::ValidatedForCalibration);
    assert(judged.aggregate.activeSectionCount == 1);
    assert(judged.aggregate.totalWindows60 == 2);

    // Weighted mean and section-level distribution retain the difficult section.
    level = validLevel();
    level.parser.sourceFrameCount = 40;
    level.parser.processedFrameCount = 40;
    level.segments.push_back(makeSegment(1, 20, 39, 4, 3, 1, 2, 0.2, 3, 2, 1));
    level.parser.clearance.meanPx = m(6, dan::EvidenceSource::ImageParser, dan::EvidenceState::Validated, 20);
    level.parser.clearance.medianPx = m(5, dan::EvidenceSource::ImageParser, dan::EvidenceState::Validated, 20);
    level.parser.clearance.minimumPx = m(1, dan::EvidenceSource::ImageParser);
    level.parser.clearance.percentile10Px = m(2.5, dan::EvidenceSource::ImageParser, dan::EvidenceState::Validated, 20);
    level.parser.frameWindows = {m(5, dan::EvidenceSource::ImageParser),
                                 m(3, dan::EvidenceSource::ImageParser),
                                 m(1, dan::EvidenceSource::ImageParser)};
    level.parser.nerves = m(0.4, dan::EvidenceSource::ImageParser, dan::EvidenceState::Validated, 20);
    judged = dan::judge(level);
    assert(judged.status == dan::JudgeStatus::ValidatedForCalibration);
    assert(judged.aggregate.activeSectionCount == 2);
    assert(judged.aggregate.sectionMeanClearance.weightedMean.value == 6.0);
    assert(judged.aggregate.sectionMeanClearance.medianOfSections.value == 4.0);
    assert(judged.aggregate.sectionMeanClearance.percentile10OfSections.value == 4.0);
    assert(judged.aggregate.sectionMinimumClearance.minimumOfSections.value == 1.0);
    assert(judged.aggregate.totalWindows60 == 5);
    assert(judged.aggregate.totalWindows120 == 3);
    assert(judged.aggregate.totalWindows240 == 1);
    assert(judged.aggregate.nervesBySection.weightedMean.value == 0.4);

    // A mismatch between parser-level and recomputed section aggregates is rejected.
    level.parser.clearance.meanPx.value = 7.0;
    assert(dan::judge(level).status == dan::JudgeStatus::Rejected);

    // Missing summary data is incomplete, not silently interpreted as zero.
    level.parser.clearance.meanPx.value = 6.0;
    level.parser.frameWindows.windows240 = {};
    assert(dan::judge(level).status == dan::JudgeStatus::InsufficientEvidence);

    // Fractional frame-window counts are invalid.
    level.parser.frameWindows.windows240 = m(1, dan::EvidenceSource::ImageParser);
    level.segments[0].frameWindows.windows120.value = 1.5;
    assert(dan::judge(level).status == dan::JudgeStatus::Rejected);

    level = validLevel();
    auto easier = dan::SwagRouteEvidence{};
    easier.routeId = 1;
    easier.kind = dan::RouteKind::Alternative;
    easier.outcome = dan::RouteOutcome::Completed;
    easier.relationToReference = dan::DifficultyRelation::Easier;
    easier.bypassedDifficultyFraction = m(0.4, dan::EvidenceSource::SwagRouteSearch);
    level.routes.push_back(easier);
    auto easierMore = easier;
    easierMore.routeId = 2;
    easierMore.bypassedDifficultyFraction = m(0.7, dan::EvidenceSource::SwagRouteSearch);
    level.routes.push_back(easierMore);
    auto harder = easier;
    harder.routeId = 3;
    harder.relationToReference = dan::DifficultyRelation::Harder;
    harder.bypassedDifficultyFraction = m(0.95, dan::EvidenceSource::SwagRouteSearch);
    level.routes.push_back(harder);
    judged = dan::judge(level);
    assert(judged.status == dan::JudgeStatus::ValidatedForCalibration);
    assert(judged.confirmedEasierRouteCount == 2);
    assert(judged.aggregate.maximumDifficultyBypassFraction.value == 0.7);

    // An unvalidated bypass estimate does not alter the route adjustment evidence.
    easier.routeId = 4;
    easier.bypassedDifficultyFraction.state = dan::EvidenceState::Measured;
    level.routes.push_back(easier);
    judged = dan::judge(level);
    assert(judged.aggregate.confirmedEasierRouteCount == 2);
    assert(judged.aggregate.maximumDifficultyBypassFraction.value == 0.7);

    // A completed easier route without validated bypass coverage blocks calibration.
    level = validLevel();
    auto unquantifiedEasier = dan::SwagRouteEvidence{};
    unquantifiedEasier.routeId = 1;
    unquantifiedEasier.kind = dan::RouteKind::Alternative;
    unquantifiedEasier.outcome = dan::RouteOutcome::Completed;
    unquantifiedEasier.relationToReference = dan::DifficultyRelation::Easier;
    level.routes.push_back(unquantifiedEasier);
    assert(dan::judge(level).status == dan::JudgeStatus::InsufficientEvidence);

    level = validLevel();
    level.segments[0].unsupportedHitboxCount = 1;
    assert(dan::judge(level).status == dan::JudgeStatus::InsufficientEvidence);

    level = validLevel();
    level.parser.processedFrameCount = 19;
    assert(dan::judge(level).status == dan::JudgeStatus::Rejected);

    level = validLevel();
    level.segments[0].clearance.minimumPx = m(12, dan::EvidenceSource::ImageParser);
    assert(dan::judge(level).status == dan::JudgeStatus::Rejected);

    level = validLevel();
    level.routeSearch = dan::RouteSearchState::Partial;
    assert(dan::judge(level).status == dan::JudgeStatus::InsufficientEvidence);

    level = validLevel();
    level.parser.nerves = m(std::numeric_limits<double>::quiet_NaN(), dan::EvidenceSource::ImageParser);
    assert(dan::judge(level).status == dan::JudgeStatus::Rejected);

    level = validLevel();
    auto duplicate = level.routes.front();
    level.routes.push_back(duplicate);
    assert(dan::judge(level).status == dan::JudgeStatus::Rejected);

    level = validLevel();
    level.segments.clear();
    assert(dan::judge(level).status == dan::JudgeStatus::Rejected);

    std::cout << "Dan analyzer aggregation and verification: all checks passed\n";
}
