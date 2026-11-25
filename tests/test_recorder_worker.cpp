#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include "recorder_worker.hpp"

namespace rec = geodan::core::recorder;

int main() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() /
                      ("geodan-worker-test-" + std::to_string(stamp));
    rec::PixelBufferPool pool(2, 2u * 1u * 4u);
    rec::BoundedFrameQueue<rec::QueuedFrame> queue(2);
    {
        rec::RecorderWorker worker(queue);
        auto pixels = pool.tryAcquire();
        if (!pixels) return EXIT_FAILURE;
        const std::uint8_t rgba[8]{255, 0, 0, 255, 0, 255, 0, 255};
        std::copy(std::begin(rgba), std::end(rgba), pixels->data());
        rec::FrameGeometry geometry;
        geometry.sequence = 0;
        geometry.phase = rec::CapturePhase::ActiveGameplay;
        geometry.status = rec::GeometryStatus::Valid;
        geometry.entities.push_back({rec::GeometryEntityKind::Player, 1, {0.0, 0.0, 1.0, 1.0}});
        geometry.entities.push_back({rec::GeometryEntityKind::Hazard, 2, {1.5, 0.0, 2.0, 1.0}});
        if (!queue.tryPush(rec::QueuedFrame{{0, 0, 0, 0}, std::move(*pixels), std::move(geometry)}))
            return EXIT_FAILURE;
        if (!worker.start(root / "session", {2, 1, 60}, 1'700'000'000'000'000'000ull))
            return EXIT_FAILURE;
        if (!worker.stop(0, "test_complete")) return EXIT_FAILURE;
        if (!worker.joined() || !worker.succeeded() || worker.framesWritten() != 1)
            return EXIT_FAILURE;
    }
    const auto frame = root / "session" / "frames" / "frame_000000.png";
    std::ifstream input(frame, std::ios::binary);
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input),
                                    std::istreambuf_iterator<char>()};
    constexpr std::uint8_t signature[8]{0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
    if (bytes.size() < 45 || !std::equal(std::begin(signature), std::end(signature), bytes.begin()))
        return EXIT_FAILURE;
    std::ifstream manifest(root / "session" / "manifest.json");
    const std::string json{std::istreambuf_iterator<char>(manifest),
                           std::istreambuf_iterator<char>()};
    if (json.find("\"status\": \"complete\"") == std::string::npos)
        return EXIT_FAILURE;
    std::ifstream hitboxInput(root / "session" / "hitboxes.csv");
    const std::string hitboxes{std::istreambuf_iterator<char>(hitboxInput),
                               std::istreambuf_iterator<char>()};
    if (hitboxes.find("0,active_gameplay,valid,player,1,0,0,1,1") == std::string::npos ||
        hitboxes.find("0,active_gameplay,valid,hazard,2,1.5,0,2,1") == std::string::npos)
        return EXIT_FAILURE;
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    std::puts("recorder worker: queue drain, PNG output and finalization checks passed");
    return EXIT_SUCCESS;
}
