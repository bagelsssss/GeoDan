// GeoDan Level Recorder - unsupported-frame reasons (portable).
// Counts, per reason and object type, the frames that were marked unsupported.
// Each frame counts once per (reason, object type). Written to
// unsupported_reasons.csv when a session stops.
#pragma once

#include "recorder_geometry.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace geodan::core::recorder {

using UnsupportedCounts = std::map<std::pair<std::string, int>, std::uint64_t>;

inline void addFrameNotes(UnsupportedCounts& counts, const std::vector<UnsupportedNote>& notes) {
    std::set<std::pair<std::string, int>> seen;
    for (const auto& note : notes) {
        const auto key = std::make_pair(note.reason, note.objectType);
        if (seen.insert(key).second) ++counts[key];
    }
}

// CSV text: header, then one row per (reason, object type), sorted.
inline std::string unsupportedCsv(const UnsupportedCounts& counts) {
    std::ostringstream out;
    out << "reason,object_type,frames\n";
    for (const auto& [key, frames] : counts)
        out << key.first << ',' << key.second << ',' << frames << '\n';
    return out.str();
}

} // namespace geodan::core::recorder
