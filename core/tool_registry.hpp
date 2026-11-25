// GeoDan Tools - the tool list and its on/off rules. Pure C++ (no Geode), so it
// is unit-tested on its own. The Geode layer only reads and writes settings.
#pragma once
#include <cstddef>
#include <string_view>

namespace geodan::core {

enum class Status {
    Available,  // implemented: can be switched on
    Planned,    // listed for the roadmap: always off, cannot be switched on
};

struct ToolDef {
    std::string_view id;       // setting key in tools/mod.json; stable, never reused
    std::string_view title;    // card heading
    std::string_view blurb;    // one line under the heading
    Status status;
    bool defaultOn;
};

inline constexpr ToolDef kTools[] = {
    {"hitboxes", "Hitboxes", "Outline each object's own hitbox in gameplay.", Status::Available, false},
    {"unclassified", "Unclassified", "Also outline objects GeoDan cannot classify, in magenta.", Status::Available, false},
    {"hitbox-trail", "Hitbox trail", "Faded grey trail of the player's recent hitboxes.", Status::Available, false},
    {"trajectory", "Trajectory", "Predicted path ahead for hold and release.", Status::Available, false},
    {"level-recorder", "Level recorder", "Records the level, tools on, to a session folder.", Status::Available, false},
    {"macro-bot", "Macro bot", "Plays a recorded input sequence. Later.", Status::Planned, false},
};

inline constexpr std::size_t kToolCount = sizeof(kTools) / sizeof(kTools[0]);

constexpr bool canEnable(const ToolDef& t) { return t.status == Status::Available; }

// Effective state: planned tools are always off, whatever is saved.
// `saved(id, fallback)` returns the stored value or the fallback.
template <typename Saved>
bool isOn(const ToolDef& t, Saved&& saved) {
    return canEnable(t) && saved(t.id, t.defaultOn);
}

template <typename Saved>
std::size_t countOn(Saved&& saved) {
    std::size_t n = 0;
    for (const auto& t : kTools)
        if (isOn(t, saved)) ++n;
    return n;
}

}  // namespace geodan::core
