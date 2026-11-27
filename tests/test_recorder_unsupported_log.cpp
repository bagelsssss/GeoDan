// Portable test for the unsupported-frame reason counts. Build:
// g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. tests/test_recorder_unsupported_log.cpp
#include "recorder_unsupported_log.hpp"

#include <cassert>
#include <cstdio>

using namespace geodan::core::recorder;

int main() {
    UnsupportedCounts counts;

    // One frame with two notes of the same reason and type counts once.
    addFrameNotes(counts, {{"unclassified", 42}, {"unclassified", 42}});
    assert(counts.size() == 1);
    assert((counts[{"unclassified", 42}]) == 1);

    // Different types and reasons count separately, across frames.
    addFrameNotes(counts, {{"unclassified", 42}, {"shape_failed", 7}});
    addFrameNotes(counts, {{"no_player", -1}});
    assert((counts[{"unclassified", 42}]) == 2);
    assert((counts[{"shape_failed", 7}]) == 1);
    assert((counts[{"no_player", -1}]) == 1);

    // Ignored-type notes are counted like any other note.
    addFrameNotes(counts, {{"ignored_type", 22}, {"ignored_type", 22}});
    assert((counts[{"ignored_type", 22}]) == 1);

    const std::string csv = unsupportedCsv(counts);
    const std::string expected =
        "reason,object_type,frames\n"
        "ignored_type,22,1\n"
        "no_player,-1,1\n"
        "shape_failed,7,1\n"
        "unclassified,42,2\n";
    assert(csv == expected);

    assert(unsupportedCsv({}) == "reason,object_type,frames\n");

    std::puts("Recorder unsupported log tests passed");
    return 0;
}
