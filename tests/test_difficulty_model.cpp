// Portable test for core/difficulty_model.hpp. Build:
// g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. tests/test_difficulty_model.cpp
#include "difficulty_model.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace geodan::core;
namespace dm = geodan::core::difficulty;

int main() {
    // Deadlocked's totals (1, 2, 2 episodes) land on 6.50 by construction.
    ii_analyzer::Section a, b;
    a.closeApproaches = {1, 1, 1};
    b.closeApproaches = {0, 1, 1};
    const std::vector<ii_analyzer::Section> sections = {a, b};
    assert(std::abs(dm::levelDifficulty(sections) - 6.50) < 1e-9);

    // Empty or quiet sections add nothing.
    ii_analyzer::Section quiet;
    assert(dm::sectionDifficulty(quiet) == 0.0);

    // Closer approaches weigh more: one 5 px episode beats one 20 px episode.
    ii_analyzer::Section close, far;
    close.closeApproaches = {1, 0, 0};
    far.closeApproaches = {0, 0, 1};
    assert(dm::sectionDifficulty(close) > dm::sectionDifficulty(far));

    // Labels: rank from the integer part, sublevel from the nearest centre.
    assert(dm::danLabel(6.50) == "6th Dan Mid");
    assert(dm::danLabel(8.00) == "8th Dan Low");
    assert(dm::danLabel(10.95) == "10th Dan High");
    assert(dm::danLabel(11.00) == "Alpha Dan Low");
    assert(dm::danLabel(17.75) == "Eta Dan Mid-high");
    assert(dm::danLabel(0.5) == "Below 1st Dan");

    std::puts("Difficulty model tests passed");
    return 0;
}
