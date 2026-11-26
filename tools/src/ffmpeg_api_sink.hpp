// Encodes RGBA frames in-process through the FFmpeg API mod (eclipse.ffmpeg-api),
// which is a required dependency of GeoDan Tools. Frames arrive top-down.
#pragma once

#include <eclipse.ffmpeg-api/include/recorder.hpp>
#include <eclipse.ffmpeg-api/include/render_settings.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#include "core/recorder_video_sink.hpp"

namespace geodan::tools::encoder {

class FfmpegApiSink final : public core::video::VideoSink {
public:
    FfmpegApiSink() = default;
    FfmpegApiSink(const FfmpegApiSink&) = delete;
    FfmpegApiSink& operator=(const FfmpegApiSink&) = delete;
    ~FfmpegApiSink() override { (void)close(); }

    bool open(const core::video::SinkConfig& c) override {
        if (m_open) return false;
        if (c.width <= 0 || c.height <= 0 || c.fps <= 0 || c.outputPath.empty()) {
            m_error = "invalid encoder configuration";
            return false;
        }
        const std::string codec = pickCodec();
        if (codec.empty()) {
            m_error = "no H.264 encoder in the FFmpeg API build";
            return false;
        }

        ffmpeg::RenderSettings settings;
        settings.m_hardwareAccelerationType = ffmpeg::HardwareAccelerationType::NONE;
        settings.m_pixelFormat = ffmpeg::PixelFormat::RGBA;
        settings.m_codec = codec;
        settings.m_colorspaceFilters.clear();
        // Frames are already top-down; the mod's default would flip them upside down.
        settings.m_doVerticalFlip = false;
        settings.m_bitrate = 12'000'000;
        settings.m_width = static_cast<std::uint32_t>(c.width);
        settings.m_height = static_cast<std::uint32_t>(c.height);
        settings.m_fps = static_cast<std::uint16_t>(c.fps);
        settings.m_outputFile = c.outputPath;

        m_frameBytes = static_cast<std::size_t>(c.width) * static_cast<std::size_t>(c.height) * 4u;
        m_path = c.outputPath;
        // init() allocates state even when it fails, so stop() is only called after success.
        auto result = m_recorder.init(settings);
        if (result.isErr()) {
            m_error = "FFmpeg API init failed: " + result.unwrapErr();
            return false;
        }
        m_open = true;
        m_frames = 0;
        m_error.clear();
        return true;
    }

    bool writeFrame(const std::uint8_t* rgba, std::size_t size) override {
        if (!m_open) { m_error = "encoder is not open"; return false; }
        if (!rgba || size != m_frameBytes) { m_error = "frame size mismatch"; return false; }
        // The FFmpeg API 1.2.2 header takes a whole vector, so the frame is copied once here.
        m_buffer.assign(rgba, rgba + size);
        auto result = m_recorder.writeFrame(m_buffer);
        if (result.isErr()) {
            m_error = "FFmpeg API frame failed: " + result.unwrapErr();
            return false;
        }
        ++m_frames;
        return true;
    }

    bool close() override {
        if (!m_open) return false;
        m_recorder.stop();   // writes the trailer; reports no error of its own
        m_open = false;
        if (m_frames == 0) { m_error = "no frames were encoded"; return false; }
        // stop() gives no status, so check the file is really there.
        std::error_code ec;
        const auto bytes = std::filesystem::file_size(m_path, ec);
        if (ec || bytes == 0) {
            m_error = "FFmpeg API wrote no video file";
            return false;
        }
        return m_error.empty();
    }

    const std::string& error() const override { return m_error; }

private:
    // Prefers the software H.264 encoder; any other H.264 encoder is the fallback.
    static std::string pickCodec() {
        const auto codecs = ffmpeg::Recorder::getAvailableCodecs();
        for (const auto& name : codecs)
            if (name == "libx264") return name;
        for (const auto& name : codecs)
            if (name.find("264") != std::string::npos) return name;
        return {};
    }

    ffmpeg::Recorder m_recorder;
    bool m_open = false;
    std::size_t m_frameBytes = 0;
    std::vector<std::uint8_t> m_buffer;
    std::uint64_t m_frames = 0;
    std::filesystem::path m_path;
    std::string m_error;
};

}  // namespace geodan::tools::encoder
