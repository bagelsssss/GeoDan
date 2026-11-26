// Portable test for the input event log. Build:
// g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. tests/test_recorder_input_log.cpp
#include "recorder_input_log.hpp"

#include <cassert>
#include <cstdio>

using namespace geodan::core::recorder;

int main() {
    // Capacity and drops.
    InputLog log(2);
    InputEvent a{10, true, 1, true};
    InputEvent b{20, false, 1, true};
    InputEvent c{30, true, 2, false};
    assert(log.push(a));
    assert(log.push(b));
    assert(!log.push(c));
    assert(log.events().size() == 2);
    assert(log.dropped() == 1);

    // CSV: header, then one row per event in order, flags as 1/0.
    const std::string csv = inputCsv({a, b, c});
    const std::string expected =
        "time_ns,down,button,player1\n"
        "10,1,1,1\n"
        "20,0,1,1\n"
        "30,1,2,0\n";
    assert(csv == expected);

    // An empty log still has the header.
    assert(inputCsv({}) == "time_ns,down,button,player1\n");

    std::puts("Recorder input log tests passed");
    return 0;
}
