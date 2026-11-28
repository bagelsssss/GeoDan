// Capture loss counts: CSV text and parsing.
#include "recorder_drops.hpp"

#include <cassert>
#include <cstdio>

using namespace geodan::core::recorder;

int main() {
    DropCounts none;
    assert(!none.any());
    assert(dropsCsv(none) ==
           "kind,count\n"
           "inputs_dropped,0\n"
           "touches_dropped,0\n"
           "no_buffer_drops,0\n");

    DropCounts some;
    some.inputsDropped = 3;
    some.touchesDropped = 40;
    some.noBufferDrops = 7;
    assert(some.any());

    // Round trip.
    DropCounts back;
    assert(parseDropsCsv(dropsCsv(some), back));
    assert(back.inputsDropped == 3 && back.touchesDropped == 40 && back.noBufferDrops == 7);

    // One count alone makes it lossy.
    DropCounts one;
    one.touchesDropped = 1;
    assert(one.any());

    // Unknown kinds are ignored so a newer recorder can add counts.
    DropCounts extra;
    assert(parseDropsCsv("kind,count\ninputs_dropped,2\nfuture_thing,9\n", extra));
    assert(extra.inputsDropped == 2);

    // Wrong header, a row with no comma, and a non-number are rejected.
    DropCounts bad;
    assert(!parseDropsCsv("name,value\ninputs_dropped,1\n", bad));
    assert(!parseDropsCsv("kind,count\ninputs_dropped\n", bad));
    assert(!parseDropsCsv("kind,count\ninputs_dropped,x\n", bad));
    assert(!parseDropsCsv("kind,count\ninputs_dropped,\n", bad));
    assert(!parseDropsCsv("", bad));

    std::puts("Recorder drops tests passed");
    return 0;
}
