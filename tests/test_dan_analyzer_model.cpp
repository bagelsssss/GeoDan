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

int main() {
    static_assert(dan::kSkillsetCount == 26);
    static_assert(static_cast<unsigned>(dan::DanTier::Tenth) == 10);

    auto level = validLevel();
    assert(dan::judge(level).status == dan::JudgeStatus::ValidatedForCalibration);

    auto idle = level.segments.front();
    idle.index = 1;
    idle.firstSampleIndex = 20;
    idle.lastSampleIndex = 25;
    idle.phase = dan::SegmentPhase::StartIdle;
    idle.clearance = {};
    idle.frameWindows = {};
    idle.nerves = {};
    level.segments.push_back(idle);
    assert(dan::judge(level).status == dan::JudgeStatus::ValidatedForCalibration);

    auto easier = dan::SwagRouteEvidence{};
    easier.routeId = 1;
    easier.kind = dan::RouteKind::Alternative;
    easier.outcome = dan::RouteOutcome::Completed;
    easier.relationToReference = dan::DifficultyRelation::Easier;
    easier.bypassedDifficultyFraction = m(0.4, dan::EvidenceSource::SwagRouteSearch);
    level.routes.push_back(easier);
    auto judged = dan::judge(level);
    assert(judged.status == dan::JudgeStatus::ValidatedForCalibration);
    assert(judged.confirmedEasierRouteCount == 1);

    auto harder = easier;
    harder.routeId = 2;
    harder.relationToReference = dan::DifficultyRelation::Harder;
    level.routes.push_back(harder);
    judged = dan::judge(level);
    assert(judged.confirmedEasierRouteCount == 1); // harder route does not inflate difficulty

    level.segments[0].clearance.meanPx = {};
    assert(dan::judge(level).status == dan::JudgeStatus::InsufficientEvidence);

    level.segments[0].clearance.meanPx = m(8, dan::EvidenceSource::ImageParser);
    level.segments[0].frameWindows.windows240 = {};
    assert(dan::judge(level).status == dan::JudgeStatus::InsufficientEvidence);
    level.segments[0].frameWindows.windows240 = m(0, dan::EvidenceSource::ImageParser);

    level.segments[0].unsupportedHitboxCount = 1;
    assert(dan::judge(level).status == dan::JudgeStatus::InsufficientEvidence);

    level.segments[0].unsupportedHitboxCount = 0;
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

    std::cout << "Dan analyzer judge model: all checks passed\n";
}
