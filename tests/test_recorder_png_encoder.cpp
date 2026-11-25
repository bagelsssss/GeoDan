#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "recorder_png_encoder.hpp"

namespace rec = geodan::core::recorder;
int main() {
    rec::RgbaPngEncoder encoder;
    const std::array<std::uint8_t, 8> pixels{255,0,0,255, 0,255,0,255};
    std::vector<std::uint8_t> png;
    if (!encoder.encode(2, 1, pixels, png)) return EXIT_FAILURE;
    constexpr std::array<std::uint8_t, 8> signature{0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
    if (png.size() < 45 || !std::equal(signature.begin(), signature.end(), png.begin()))
        return EXIT_FAILURE;
    if (png[12] != 'I' || png[13] != 'H' || png[14] != 'D' || png[15] != 'R')
        return EXIT_FAILURE;
    if (png[16] != 0 || png[17] != 0 || png[18] != 0 || png[19] != 2 ||
        png[20] != 0 || png[21] != 0 || png[22] != 0 || png[23] != 1)
        return EXIT_FAILURE;
    if (png[25] != 6) return EXIT_FAILURE;
    std::vector<std::uint8_t> invalid;
    if (encoder.encode(0, 1, pixels, invalid)) return EXIT_FAILURE;
    if (encoder.encode(2, 1, std::span<const std::uint8_t>(pixels.data(), 7), invalid))
        return EXIT_FAILURE;
    std::puts("recorder PNG encoder: basic structure checks passed");
    return EXIT_SUCCESS;
}
