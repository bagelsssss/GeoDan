// GeoDan Input/Image Analyzer - input log reader and checks (portable).
// Reads inputs.csv written by the level recorder (columns: time_ns,down,button,player1)
// and checks it is internally consistent before the I/I Analyzer compares it with video.
#pragma once

#include <charconv>
#include <sstream>
#include <system_error>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace geodan::core::input_session {

struct InputRecord {
    std::uint64_t timeNs = 0;
    bool down = false;
    int button = 0;
    bool player1 = true;
};

inline bool parseU64(const std::string& text, std::uint64_t& value) {
    if (text.empty()) return false;
    const auto* first = text.data();
    const auto* last = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(first, last, value);
    return ec == std::errc{} && ptr == last;
}

inline bool parseInt(const std::string& text, int& value) {
    if (text.empty()) return false;
    const auto* first = text.data();
    const auto* last = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(first, last, value);
    return ec == std::errc{} && ptr == last;
}

// Parses inputs.csv text. Returns nullopt and sets error on any malformed line.
inline std::optional<std::vector<InputRecord>> parseInputCsv(const std::string& text,
                                                             std::string& error) {
    std::vector<InputRecord> records;
    std::istringstream in(text);
    std::string line;
    if (!std::getline(in, line) || line != "time_ns,down,button,player1") {
        error = "inputs_header_invalid";
        return std::nullopt;
    }
    std::size_t lineNumber = 1;
    while (std::getline(in, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::vector<std::string> fields;
        std::size_t start = 0;
        while (true) {
            const auto comma = line.find(',', start);
            fields.push_back(line.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
            if (comma == std::string::npos) break;
            start = comma + 1;
        }
        if (fields.size() != 4) { error = "inputs_row_field_count_line_" + std::to_string(lineNumber); return std::nullopt; }
        InputRecord r;
        int down = 0, player = 0;
        if (!parseU64(fields[0], r.timeNs) || !parseInt(fields[1], down) ||
            !parseInt(fields[2], r.button) || !parseInt(fields[3], player) ||
            (down != 0 && down != 1) || (player != 0 && player != 1)) {
            error = "inputs_row_invalid_line_" + std::to_string(lineNumber);
            return std::nullopt;
        }
        r.down = down == 1;
        r.player1 = player == 1;
        records.push_back(r);
    }
    return records;
}

inline std::optional<std::vector<InputRecord>> readInputCsv(const std::filesystem::path& path,
                                                            std::string& error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) { error = "cannot_open_inputs"; return std::nullopt; }
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parseInputCsv(text, error);
}

// Checks the log against the session: times never go backwards, no event is
// after the session end, and for each button and player the presses and
// releases alternate. The first event of a button may be either a press or a
// release, because a button can be held when recording starts.
inline bool checkInputs(const std::vector<InputRecord>& records, std::uint64_t sessionEndNs,
                        std::string& error) {
    std::uint64_t previous = 0;
    std::map<std::pair<int, bool>, bool> lastDown;
    for (const auto& r : records) {
        if (r.timeNs < previous) { error = "inputs_time_not_ordered"; return false; }
        if (r.timeNs > sessionEndNs) { error = "inputs_after_session_end"; return false; }
        previous = r.timeNs;
        const auto key = std::make_pair(r.button, r.player1);
        const auto it = lastDown.find(key);
        if (it != lastDown.end() && it->second == r.down) {
            error = "inputs_do_not_alternate";
            return false;
        }
        lastDown[key] = r.down;
    }
    return true;
}

} // namespace geodan::core::input_session
