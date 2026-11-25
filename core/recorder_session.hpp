// Portable recording-session output. No Geode/Cocos/OpenGL dependencies.
// Own this writer on the encoder/writer worker thread; never call it from the
// gameplay or render thread. It accepts already-encoded PNG bytes.
#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <span>
#include <sstream>
#include <string>
#include <system_error>
#include "recorder_model.hpp"

namespace geodan::core::recorder {
struct SessionConfig {
    std::uint32_t width = kDefaultWidth;
    std::uint32_t height = kDefaultHeight;
    std::uint32_t targetFramesPerSecond = kDefaultFramesPerSecond;
    bool valid() const { return width > 0 && height > 0 && targetFramesPerSecond > 0; }
};

class SessionWriter {
public:
    enum class State { Idle, Recording, Complete, Incomplete, Failed };
    SessionWriter() = default;
    SessionWriter(const SessionWriter&) = delete;
    SessionWriter& operator=(const SessionWriter&) = delete;
    ~SessionWriter() {
        if (m_state == State::Recording) abort("writer_destroyed_before_finalize", nowUnixNs());
    }

    // Session directory must not already exist. Parent directories may.
    bool start(const std::filesystem::path& sessionDirectory,
               SessionConfig config, std::uint64_t startUnixNs) {
        if (m_state != State::Idle || !config.valid() || sessionDirectory.empty()) return false;
        std::error_code ec;
        const auto parent = sessionDirectory.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, ec);
            if (ec) return fail("cannot_create_parent_directory: " + ec.message());
        }
        if (!std::filesystem::create_directory(sessionDirectory, ec) || ec)
            return fail("cannot_create_new_session_directory: " + ec.message());
        m_sessionDirectory = sessionDirectory;
        m_framesDirectory = m_sessionDirectory / "frames";
        std::filesystem::create_directory(m_framesDirectory, ec);
        if (ec) return fail("cannot_create_frames_directory: " + ec.message());

        m_config = config;
        m_startUnixNs = startUnixNs;
        m_state = State::Recording;
        m_csv.open(m_sessionDirectory / "frames.csv",
                   std::ios::out | std::ios::binary | std::ios::trunc);
        if (!m_csv) return fail("cannot_open_frames_csv");
        m_csv << "sequence,timestamp_ns,delta_ns,missed_intervals,write_status\n";
        m_csv.flush();
        if (!m_csv) return fail("cannot_write_frames_csv_header");
        if (!writeManifest("recording", 0, "")) return fail("cannot_write_initial_manifest");
        return true;
    }

    // Sequence gaps are preserved in filenames; frames are never renumbered.
    bool writeFrame(const FrameStamp& stamp, std::span<const std::uint8_t> pngBytes) {
        if (m_state != State::Recording || !hasPngSignature(pngBytes)) return false;
        if (!m_hasLastFrame) {
            // The first persisted frame may follow one or more captured frames
            // dropped before reaching the writer. Only sequence zero must have
            // a zero delta; later first-persisted frames retain their real delta.
            if ((stamp.sequence == 0 && stamp.deltaNs != 0) ||
                (stamp.sequence > 0 && stamp.deltaNs == 0)) return false;
        } else {
            if (stamp.sequence <= m_lastSequence || stamp.timestampNs <= m_lastTimestampNs)
                return false;
            const auto persistedGapNs = stamp.timestampNs - m_lastTimestampNs;
            if (stamp.deltaNs == 0 || stamp.deltaNs > persistedGapNs) return false;
            if (stamp.sequence - m_lastSequence == 1 && stamp.deltaNs != persistedGapNs)
                return false;
        }

        const auto finalPath = m_framesDirectory / frameFilename(stamp.sequence);
        auto temporaryPath = finalPath;
        temporaryPath += ".tmp";
        {
            std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
            if (!output) return fail("cannot_open_frame_file");
            output.write(reinterpret_cast<const char*>(pngBytes.data()),
                         static_cast<std::streamsize>(pngBytes.size()));
            output.flush();
            if (!output) {
                output.close();
                std::error_code ignored;
                std::filesystem::remove(temporaryPath, ignored);
                return fail("cannot_write_frame_file");
            }
        }
        std::error_code ec;
        std::filesystem::rename(temporaryPath, finalPath, ec);
        if (ec) {
            std::error_code ignored;
            std::filesystem::remove(temporaryPath, ignored);
            return fail("cannot_finalize_frame_file: " + ec.message());
        }

        m_csv << stamp.sequence << ',' << stamp.timestampNs << ','
              << stamp.deltaNs << ',' << stamp.missedIntervals << ",written\n";
        m_csv.flush();
        if (!m_csv) {
            std::filesystem::remove(finalPath, ec);
            return fail("cannot_write_frame_metadata");
        }
        if (!m_hasLastFrame) m_firstSequence = stamp.sequence;
        m_hasLastFrame = true;
        m_lastSequence = stamp.sequence;
        m_lastTimestampNs = stamp.timestampNs;
        ++m_framesWritten;
        return true;
    }

    // Called by the worker with aggregate capture-timeline and queue counters
    // after it has drained the producer queue. This includes drops that never
    // became persisted frame rows.
    void recordMissedCaptureIntervals(std::uint64_t count) {
        m_missedIntervals = saturatingAdd(m_missedIntervals, count);
    }
    void recordQueueDrops(std::uint64_t count = 1) {
        m_queueDrops = saturatingAdd(m_queueDrops, count);
    }

    bool finalize(std::uint64_t endUnixNs, const std::string& stopReason = "user_stop") {
        if (m_state != State::Recording) return false;
        m_csv.flush();
        const bool csvOkay = static_cast<bool>(m_csv);
        m_csv.close();
        if (!csvOkay) return fail("frames_csv_flush_failed");
        if (!writeManifest("complete", endUnixNs, stopReason))
            return fail("cannot_finalize_manifest");
        m_state = State::Complete;
        return true;
    }

    bool abort(const std::string& reason, std::uint64_t endUnixNs) {
        if (m_state != State::Recording) return false;
        m_csv.flush();
        const bool csvOkay = static_cast<bool>(m_csv);
        m_csv.close();
        const bool manifestOkay = writeManifest(csvOkay ? "incomplete" : "error", endUnixNs, reason);
        m_state = csvOkay && manifestOkay ? State::Incomplete : State::Failed;
        if (!manifestOkay) m_error = "cannot_write_abort_manifest";
        return m_state == State::Incomplete;
    }

    State state() const { return m_state; }
    std::uint64_t framesWritten() const { return m_framesWritten; }
    std::uint64_t missedIntervals() const { return m_missedIntervals; }
    std::uint64_t queueDrops() const { return m_queueDrops; }
    const std::string& error() const { return m_error; }
    const std::filesystem::path& directory() const { return m_sessionDirectory; }

    static std::uint64_t nowUnixNs() {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
    }

private:
    static constexpr std::array<std::uint8_t, 8> kPngSignature{
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
    };
    static bool hasPngSignature(std::span<const std::uint8_t> bytes) {
        return bytes.size() >= kPngSignature.size() &&
               std::equal(kPngSignature.begin(), kPngSignature.end(), bytes.begin());
    }
    static std::uint64_t saturatingAdd(std::uint64_t a, std::uint64_t b) {
        return b > std::numeric_limits<std::uint64_t>::max() - a
            ? std::numeric_limits<std::uint64_t>::max() : a + b;
    }
    static std::string frameFilename(std::uint64_t sequence) {
        std::ostringstream name;
        name << "frame_" << std::setw(6) << std::setfill('0') << sequence << ".png";
        return name.str();
    }
    static std::string jsonEscape(const std::string& value) {
        std::ostringstream out;
        for (const unsigned char ch : value) {
            switch (ch) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (ch < 0x20) {
                        constexpr char hex[] = "0123456789abcdef";
                        out << "\\u00" << hex[(ch >> 4) & 0xf] << hex[ch & 0xf];
                    } else out << static_cast<char>(ch);
            }
        }
        return out.str();
    }
    bool writeManifest(const std::string& status, std::uint64_t endUnixNs,
                       const std::string& stopReason) {
        std::ostringstream json;
        json << "{\n"
             << "  \"schema_version\": 1,\n"
             << "  \"status\": \"" << status << "\",\n"
             << "  \"width\": " << m_config.width << ",\n"
             << "  \"height\": " << m_config.height << ",\n"
             << "  \"target_fps\": " << m_config.targetFramesPerSecond << ",\n"
             << "  \"frames_written\": " << m_framesWritten << ",\n"
             << "  \"missed_capture_intervals\": " << m_missedIntervals << ",\n"
             << "  \"writer_queue_drops\": " << m_queueDrops << ",\n"
             << "  \"start_unix_ns\": " << m_startUnixNs << ",\n"
             << "  \"end_unix_ns\": ";
        if (endUnixNs == 0) json << "null"; else json << endUnixNs;
        json << ",\n  \"first_sequence\": ";
        if (!m_hasLastFrame) json << "null"; else json << m_firstSequence;
        json << ",\n  \"last_sequence\": ";
        if (!m_hasLastFrame) json << "null"; else json << m_lastSequence;
        json << ",\n  \"stop_reason\": \"" << jsonEscape(stopReason) << "\",\n"
             << "  \"frames_metadata\": \"frames.csv\",\n"
             << "  \"frames_directory\": \"frames\"\n}\n";

        const auto temporary = m_sessionDirectory / "manifest.json.tmp";
        const auto destination = m_sessionDirectory / "manifest.json";
        {
            std::ofstream output(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!output) return false;
            const auto contents = json.str();
            output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
            output.flush();
            if (!output) return false;
        }
        std::error_code ec;
        std::filesystem::rename(temporary, destination, ec);
        if (ec) {
            // Windows does not replace an existing destination on rename.
            std::filesystem::remove(destination, ec);
            ec.clear();
            std::filesystem::rename(temporary, destination, ec);
        }
        return !ec;
    }
    bool fail(const std::string& message) {
        const bool wasRecording = m_state == State::Recording;
        m_error = message;
        if (m_csv.is_open()) {
            m_csv.flush();
            m_csv.close();
        }
        if (wasRecording && !m_sessionDirectory.empty())
            (void)writeManifest("error", nowUnixNs(), message);
        m_state = State::Failed;
        return false;
    }

    State m_state = State::Idle;
    SessionConfig m_config;
    std::filesystem::path m_sessionDirectory;
    std::filesystem::path m_framesDirectory;
    std::ofstream m_csv;
    std::uint64_t m_startUnixNs = 0, m_framesWritten = 0;
    std::uint64_t m_missedIntervals = 0, m_queueDrops = 0;
    std::uint64_t m_firstSequence = 0, m_lastSequence = 0, m_lastTimestampNs = 0;
    bool m_hasLastFrame = false;
    std::string m_error;
};
} // namespace geodan::core::recorder
