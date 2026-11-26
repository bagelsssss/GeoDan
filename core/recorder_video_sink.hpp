// Streams raw RGBA frames into an ffmpeg process that encodes an H.264 MP4.
// Portable (no Geode, no Cocos). The game thread never calls this: the recorder
// worker owns the sink and writes one frame per written capture, in order.
//
// PC only for now. Android needs an in-process encoder later; the sink is
// deliberately small so that swap only touches this file.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#if defined(_WIN32)
#define GEODAN_POPEN _popen
#define GEODAN_PCLOSE _pclose
#else
#define GEODAN_POPEN popen
#define GEODAN_PCLOSE pclose
#endif

namespace geodan::core::video {

struct SinkConfig {
    std::string ffmpeg = "ffmpeg";  // executable name or full path
    std::string outputPath;         // .mp4 file to create
    int width = 0;
    int height = 0;
    int fps = 60;                   // nominal rate; the CSV holds the real timing
};

// Paths and the executable are quoted for the shell. Quotes inside them cannot
// be escaped portably, so they are rejected instead.
inline bool safeArgument(const std::string& s) {
    return !s.empty() && s.find('"') == std::string::npos && s.find('\n') == std::string::npos;
}

inline bool validConfig(const SinkConfig& c) {
    return safeArgument(c.ffmpeg) && safeArgument(c.outputPath) &&
           c.width > 0 && c.height > 0 && c.fps > 0;
}

// The full command: raw RGBA on stdin, constant-rate H.264 MP4 out.
// Returns "" for an invalid config.
inline std::string buildCommand(const SinkConfig& c) {
    if (!validConfig(c)) return {};
    return "\"" + c.ffmpeg + "\" -y -loglevel error" +
           " -f rawvideo -pix_fmt rgba -s " + std::to_string(c.width) + "x" +
           std::to_string(c.height) + " -r " + std::to_string(c.fps) + " -i -" +
           " -an -c:v libx264 -preset veryfast -crf 23 -pix_fmt yuv420p" +
           " \"" + c.outputPath + "\"";
}

class FfmpegSink final {
public:
    FfmpegSink() = default;
    FfmpegSink(const FfmpegSink&) = delete;
    FfmpegSink& operator=(const FfmpegSink&) = delete;
    ~FfmpegSink() { close(); }

    bool open(const SinkConfig& c) {
        if (m_pipe) return false;
        const std::string cmd = buildCommand(c);
        if (cmd.empty()) { m_error = "invalid sink configuration"; return false; }
        m_frameBytes = static_cast<std::size_t>(c.width) * static_cast<std::size_t>(c.height) * 4u;
        m_pipe = GEODAN_POPEN(cmd.c_str(), "w");
        if (!m_pipe) { m_error = "could not start ffmpeg"; return false; }
        m_frames = 0;
        m_error.clear();
        return true;
    }

    // One frame of exactly width*height*4 bytes, rows top-down.
    bool writeFrame(const std::uint8_t* rgba, std::size_t size) {
        if (!m_pipe) { m_error = "sink is not open"; return false; }
        if (!rgba || size != m_frameBytes) { m_error = "frame size mismatch"; return false; }
        if (std::fwrite(rgba, 1, size, m_pipe) != size) {
            m_error = "ffmpeg stopped accepting frames";
            return false;
        }
        ++m_frames;
        return true;
    }

    // Ends the stream and waits for ffmpeg to finish writing the file.
    // Returns true only when ffmpeg exited cleanly.
    bool close() {
        if (!m_pipe) return m_frames > 0 && m_error.empty();
        const int status = GEODAN_PCLOSE(m_pipe);
        m_pipe = nullptr;
        if (status != 0) m_error = "ffmpeg exited with status " + std::to_string(status);
        return status == 0;
    }

    std::uint64_t framesWritten() const { return m_frames; }
    const std::string& error() const { return m_error; }

private:
    std::FILE* m_pipe = nullptr;
    std::size_t m_frameBytes = 0;
    std::uint64_t m_frames = 0;
    std::string m_error;
};

}  // namespace geodan::core::video
