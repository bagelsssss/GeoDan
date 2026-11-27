// GeoDan difficulty model (provisional). Portable, no Geode.
//
// A section's difficulty is a weighted count of its close-approach episodes, with
// closer thresholds weighted more. The level score is the sum over sections. The
// scale is fitted so Deadlocked's sum is 6.50 (6th Dan Mid), the only anchor with
// an exact value. Other anchor values below are provisional guesses, not measured.
#pragma once

#include "input_image_analyzer.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace geodan::core::difficulty {

// Weights for the close-approach thresholds: 5 px, 10 px, 20 px.
inline constexpr std::array<double, 3> kThresholdWeights = {3.0, 2.0, 1.0};

// Deadlocked's close-approach episodes summed over its sections: 5 px 1, 10 px 2, 20 px 2.
inline constexpr std::array<std::uint32_t, 3> kDeadlockedCloseTotals = {1, 2, 2};
inline constexpr double kDeadlockedDan = 6.50;

inline constexpr double kRawDeadlocked = kThresholdWeights[0] * kDeadlockedCloseTotals[0]
    + kThresholdWeights[1] * kDeadlockedCloseTotals[1]
    + kThresholdWeights[2] * kDeadlockedCloseTotals[2];

// Scale that makes Deadlocked's raw total land on 6.50.
inline constexpr double kScale = kDeadlockedDan / kRawDeadlocked;

// Provisional anchor values, for reference. Change these when real numbers are agreed.
struct Anchor {
    const char* name;
    double value;
};
inline constexpr std::array<Anchor, 5> kAnchors = {{
    {"Deadlocked", 6.50},
    {"8th -- (Windy Landscape, Acropolis)", 8.00},
    {"Alpha -- (Acu)", 11.00},
    {"10th ++ (In Canon, Ice Carbon Diablo X)", 10.95},
    {"Eta + (Grief)", 17.75},
}};

inline double sectionDifficulty(const ii_analyzer::Section& s) {
    double raw = 0.0;
    for (std::size_t t = 0; t < kThresholdWeights.size(); ++t)
        raw += kThresholdWeights[t] * static_cast<double>(s.closeApproaches[t]);
    return kScale * raw;
}

inline double levelDifficulty(const std::vector<ii_analyzer::Section>& sections) {
    double total = 0.0;
    for (const auto& s : sections) total += sectionDifficulty(s);
    return total;
}

// Dan label for a value: rank from the integer part (1-10 ordinal, 11-20 Greek),
// sublevel from the nearest centre (-- .05, - .25, mid .50, + .75, ++ .95).
inline std::string danLabel(double value) {
    if (value < 1.0) return "Below 1st Dan";
    const int rank = static_cast<int>(std::floor(value));
    if (rank > 20) return "Above Kappa (" + std::to_string(value).substr(0, 5) + ")";
    static const char* ordinals[] = {"", "1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th", "10th"};
    static const char* greek[] = {"", "", "", "", "", "", "", "", "", "", "",
        "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta", "Iota", "Kappa"};
    const std::string name = rank <= 10 ? ordinals[rank] : greek[rank];

    const double frac = value - rank;
    struct Sub { double centre; const char* word; };
    static constexpr Sub subs[] = {
        {0.05, "Low"}, {0.25, "Mid-low"}, {0.50, "Mid"}, {0.75, "Mid-high"}, {0.95, "High"}};
    const Sub* best = &subs[0];
    for (const auto& s : subs)
        if (std::abs(frac - s.centre) < std::abs(frac - best->centre)) best = &s;
    return name + " Dan " + best->word;
}

} // namespace geodan::core::difficulty
