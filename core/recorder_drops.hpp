// GeoDan Level Recorder - capture loss counts.
// Inputs, touches and frames can be dropped when a log or queue is full. The counts
// go into capture_drops.csv in the session folder so the analyzer can tell a lossy
// session from a clean one. A session without the file predates it (unknown).
#pragma once

#include <cstdint>
#include <sstream>
#include <string>

namespace geodan::core::recorder {

struct DropCounts {
    std::uint64_t inputsDropped = 0;   // input events refused because the log was full
    std::uint64_t touchesDropped = 0;  // touch events refused because the log was full
    std::uint64_t noBufferDrops = 0;   // frames skipped because no pixel buffer was free
    bool any() const { return inputsDropped > 0 || touchesDropped > 0 || noBufferDrops > 0; }
};

// CSV text with columns kind,count and one row per count.
inline std::string dropsCsv(const DropCounts& d) {
    std::ostringstream out;
    out << "kind,count\n"
        << "inputs_dropped," << d.inputsDropped << '\n'
        << "touches_dropped," << d.touchesDropped << '\n'
        << "no_buffer_drops," << d.noBufferDrops << '\n';
    return out.str();
}

// Reads the CSV back. Returns false when the header is wrong or a row is malformed.
// Unknown kinds are ignored so a newer recorder can add counts.
inline bool parseDropsCsv(const std::string& text, DropCounts& out) {
    std::istringstream lines(text);
    std::string line;
    if (!std::getline(lines, line) || line != "kind,count") return false;
    DropCounts result;
    while (std::getline(lines, line)) {
        if (line.empty()) continue;
        const auto comma = line.find(',');
        if (comma == std::string::npos) return false;
        const std::string kind = line.substr(0, comma);
        const std::string value = line.substr(comma + 1);
        if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) return false;
        const std::uint64_t count = std::stoull(value);
        if (kind == "inputs_dropped") result.inputsDropped = count;
        else if (kind == "touches_dropped") result.touchesDropped = count;
        else if (kind == "no_buffer_drops") result.noBufferDrops = count;
    }
    out = result;
    return true;
}

} // namespace geodan::core::recorder
