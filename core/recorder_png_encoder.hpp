// Portable RGBA8 -> PNG encoder using zlib DEFLATE stored blocks.
// No external compression library or Geode dependency is required.
// This favors predictable CPU cost over compression ratio; frames remain valid PNG.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace geodan::core::recorder {

class RgbaPngEncoder final {
public:
    bool encode(std::uint32_t width, std::uint32_t height,
                std::span<const std::uint8_t> rgba,
                std::vector<std::uint8_t>& png) const {
        png.clear();
        if (width == 0 || height == 0 ||
            width > 0x7fffffffu || height > 0x7fffffffu)
            return false;

        constexpr std::size_t channels = 4;
        if (static_cast<std::uint64_t>(width) >
            (std::numeric_limits<std::size_t>::max() - 1u) / channels)
            return false;
        const std::size_t rowBytes = static_cast<std::size_t>(width) * channels;
        if (static_cast<std::uint64_t>(height) >
            std::numeric_limits<std::size_t>::max() / rowBytes)
            return false;
        const std::size_t expected = rowBytes * static_cast<std::size_t>(height);
        if (rgba.size() != expected) return false;
        if (static_cast<std::size_t>(height) >
            (std::numeric_limits<std::size_t>::max() / (rowBytes + 1u)))
            return false;

        const std::size_t filteredSize = (rowBytes + 1u) * static_cast<std::size_t>(height);
        std::vector<std::uint8_t> filtered;
        std::vector<std::uint8_t> compressed;
        try {
            filtered.reserve(filteredSize);
            for (std::uint32_t y = 0; y < height; ++y) {
                filtered.push_back(0); // PNG filter type 0: None
                const auto offset = static_cast<std::size_t>(y) * rowBytes;
                filtered.insert(filtered.end(), rgba.begin() + static_cast<std::ptrdiff_t>(offset),
                                rgba.begin() + static_cast<std::ptrdiff_t>(offset + rowBytes));
            }
            if (!makeZlibStoredStream(filtered, compressed)) return false;

            png.reserve(8u + 25u + compressed.size() + 12u);
            constexpr std::array<std::uint8_t, 8> signature{
                0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
            };
            png.insert(png.end(), signature.begin(), signature.end());

            std::array<std::uint8_t, 13> ihdr{};
            writeBigEndian32(ihdr.data(), width);
            writeBigEndian32(ihdr.data() + 4, height);
            ihdr[8] = 8; // bit depth
            ihdr[9] = 6; // RGBA
            ihdr[10] = 0;
            ihdr[11] = 0;
            ihdr[12] = 0;
            appendChunk(png, "IHDR", ihdr);
            appendChunk(png, "IDAT", compressed);
            appendChunk(png, "IEND", {});
        } catch (...) {
            png.clear();
            return false;
        }
        return true;
    }

private:
    static void writeBigEndian32(std::uint8_t* out, std::uint32_t value) {
        out[0] = static_cast<std::uint8_t>(value >> 24);
        out[1] = static_cast<std::uint8_t>(value >> 16);
        out[2] = static_cast<std::uint8_t>(value >> 8);
        out[3] = static_cast<std::uint8_t>(value);
    }

    static std::uint32_t crc32(std::span<const std::uint8_t> bytes) {
        std::uint32_t crc = 0xffffffffu;
        for (const auto byte : bytes) {
            crc ^= byte;
            for (int bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
        return ~crc;
    }

    static void appendChunk(std::vector<std::uint8_t>& out, const char* type,
                            std::span<const std::uint8_t> data) {
        const auto length = static_cast<std::uint32_t>(data.size());
        const auto oldSize = out.size();
        out.resize(oldSize + 12u + data.size());
        writeBigEndian32(out.data() + oldSize, length);
        for (std::size_t i = 0; i < 4; ++i)
            out[oldSize + 4u + i] = static_cast<std::uint8_t>(type[i]);
        for (std::size_t i = 0; i < data.size(); ++i)
            out[oldSize + 8u + i] = data[i];
        const auto crc = crc32(std::span<const std::uint8_t>(
            out.data() + oldSize + 4u, 4u + data.size()));
        writeBigEndian32(out.data() + oldSize + 8u + data.size(), crc);
    }

    static bool makeZlibStoredStream(std::span<const std::uint8_t> input,
                                     std::vector<std::uint8_t>& output) {
        output.clear();
        if (input.size() > std::numeric_limits<std::size_t>::max() - 6u) return false;
        output.reserve(input.size() + (input.size() / 65535u + 1u) * 5u + 6u);
        output.push_back(0x78); // zlib, DEFLATE, 32K window
        output.push_back(0x01); // fastest/no-compression profile
        std::size_t offset = 0;
        do {
            const auto remaining = input.size() - offset;
            const auto blockSize = remaining > 65535u ? 65535u : remaining;
            const bool finalBlock = offset + blockSize == input.size();
            output.push_back(finalBlock ? 0x01 : 0x00); // BFINAL + stored block
            const auto len = static_cast<std::uint16_t>(blockSize);
            const auto inverse = static_cast<std::uint16_t>(~len);
            output.push_back(static_cast<std::uint8_t>(len));
            output.push_back(static_cast<std::uint8_t>(len >> 8));
            output.push_back(static_cast<std::uint8_t>(inverse));
            output.push_back(static_cast<std::uint8_t>(inverse >> 8));
            output.insert(output.end(),
                          input.begin() + static_cast<std::ptrdiff_t>(offset),
                          input.begin() + static_cast<std::ptrdiff_t>(offset + blockSize));
            offset += blockSize;
        } while (offset < input.size());

        std::uint32_t a = 1, b = 0;
        for (const auto byte : input) {
            a = (a + byte) % 65521u;
            b = (b + a) % 65521u;
        }
        const std::uint32_t adler = (b << 16) | a;
        const auto oldSize = output.size();
        output.resize(oldSize + 4u);
        writeBigEndian32(output.data() + oldSize, adler);
        return true;
    }
};

} // namespace geodan::core::recorder
