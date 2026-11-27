// Portable test for the input log reader and checks. Build:
// g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. tests/test_input_session.cpp
#include "input_session.hpp"

#include <cassert>
#include <cstdio>

using namespace geodan::core::input_session;

int main() {
    std::string error;

    // A good log parses and passes the checks.
    const std::string good =
        "time_ns,down,button,player1\n"
        "100,1,1,1\n"
        "200,0,1,1\n"
        "250,1,1,0\n"
        "300,0,1,0\n";
    auto records = parseInputCsv(good, error);
    assert(records && records->size() == 4);
    assert(checkInputs(*records, 1000, error));

    // Header must match exactly.
    assert(!parseInputCsv("time,down\n100,1\n", error));
    assert(error == "inputs_header_invalid");

    // Field count and value ranges.
    assert(!parseInputCsv("time_ns,down,button,player1\n100,1,1\n", error));
    assert(!parseInputCsv("time_ns,down,button,player1\n100,2,1,1\n", error));
    assert(!parseInputCsv("time_ns,down,button,player1\n-5,1,1,1\n", error));

    // Times going backwards are rejected.
    auto backwards = parseInputCsv("time_ns,down,button,player1\n200,1,1,1\n100,0,1,1\n", error);
    assert(backwards && !checkInputs(*backwards, 1000, error));
    assert(error == "inputs_time_not_ordered");

    // An event after the session end is rejected.
    auto late = parseInputCsv("time_ns,down,button,player1\n1200,1,1,1\n", error);
    assert(late && !checkInputs(*late, 1000, error));
    assert(error == "inputs_after_session_end");

    // Two presses in a row for the same button and player are rejected.
    auto doubled = parseInputCsv("time_ns,down,button,player1\n100,1,1,1\n200,1,1,1\n", error);
    assert(doubled && !checkInputs(*doubled, 1000, error));
    assert(error == "inputs_do_not_alternate");

    // A release first is allowed (button held when recording started).
    auto heldAtStart = parseInputCsv("time_ns,down,button,player1\n50,0,1,1\n100,1,1,1\n", error);
    assert(heldAtStart && checkInputs(*heldAtStart, 1000, error));

    // Single-player: a press and its release with different player flags still pair.
    auto mixedFlags = parseInputCsv("time_ns,down,button,player1\n100,1,1,0\n200,0,1,1\n", error);
    assert(mixedFlags && checkInputs(*mixedFlags, 1000, error, false));
    assert(mixedFlags && checkInputs(*mixedFlags, 1000, error, true));
    // Two presses with different player flags: a double press in single-player,
    // but two players pressing the same button in dual mode.
    auto twoPlayers = parseInputCsv("time_ns,down,button,player1\n100,1,1,0\n200,1,1,1\n", error);
    assert(twoPlayers && !checkInputs(*twoPlayers, 1000, error, false));
    assert(error == "inputs_do_not_alternate");
    error.clear();
    assert(twoPlayers && checkInputs(*twoPlayers, 1000, error, true));

    // Header-only log is valid and empty.
    auto empty = parseInputCsv("time_ns,down,button,player1\n", error);
    assert(empty && empty->empty() && checkInputs(*empty, 0, error));

    std::puts("Input session tests passed");
    return 0;
}
