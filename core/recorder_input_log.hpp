// GeoDan Level Recorder - input event log (portable).
// Records every button press and release the game reports, with the time since
// the session started, so the I/I Analyzer can compare the player's input with
// the recorded video. Game thread only; no locking, no I/O here.
#pragma once

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace geodan::core::recorder {

inline constexpr std::size_t kDefaultInputCapacity = 200000;

struct InputEvent {
    std::uint64_t timeNs = 0; // nanoseconds since the session started
    bool down = false;        // true = press, false = release
    int button = 0;           // the game's button index
    bool player1 = true;      // true = player 1, false = player 2
};

class InputLog {
public:
    explicit InputLog(std::size_t capacity = kDefaultInputCapacity) : m_capacity(capacity) {}

    // Returns false and counts a drop when the log is full. Never grows past capacity.
    bool push(const InputEvent& event) {
        if (m_events.size() >= m_capacity) {
            ++m_dropped;
            return false;
        }
        m_events.push_back(event);
        return true;
    }

    const std::vector<InputEvent>& events() const { return m_events; }
    std::uint64_t dropped() const { return m_dropped; }

private:
    std::size_t m_capacity;
    std::vector<InputEvent> m_events;
    std::uint64_t m_dropped = 0;
};

// CSV text: a header line, then one row per event in the order they happened.
// Columns: time_ns,down,button,player1 (down and player1 are 1 or 0).
inline std::string inputCsv(const std::vector<InputEvent>& events) {
    std::ostringstream out;
    out << "time_ns,down,button,player1\n";
    for (const auto& e : events) {
        out << e.timeNs << ',' << (e.down ? 1 : 0) << ',' << e.button << ','
            << (e.player1 ? 1 : 0) << '\n';
    }
    return out.str();
}

} // namespace geodan::core::recorder
