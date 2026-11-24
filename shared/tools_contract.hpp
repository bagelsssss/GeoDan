// GeoDan Tools - shared contract (Geode-free, unit-tested).
// The Tools mod owns these IDs and setting keys. Other GeoDan mods refer to
// them through this header only; they never link against the Tools mod.
#pragma once
#include <cstddef>
#include <string_view>

namespace geodan::tools {

inline constexpr std::string_view kModID = "local.geodan-tools";

struct Toggle {
    std::string_view key;      // bool setting in tools/mod.json
    std::string_view label;    // shown in the Tools popup
    bool implemented;          // false = listed as a planned tool, cannot be turned on yet
};

// Order = order in the popup.
inline constexpr Toggle kToggles[] = {
    {"show-hitboxes",       "HITBOXES",          true},
    {"show-unclassified",   "UNCLASSIFIED",      true},
    {"show-hitbox-trail",   "HITBOX TRAIL",      true},
    {"show-trajectory",     "TRAJECTORY",        true},
    {"record-level",        "LEVEL RECORDER",    false},
    {"macro-bot",           "MACRO BOT",         false},
};

inline constexpr std::size_t kToggleCount = sizeof(kToggles) / sizeof(kToggles[0]);

// Whether a toggle may be switched on. Planned tools stay off.
constexpr bool canEnable(const Toggle& t) { return t.implemented; }

}  // namespace geodan::tools
