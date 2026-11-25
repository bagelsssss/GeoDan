#include "image_parser.hpp"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

using namespace geodan::core::image_parser;

static void writePngHeader(const std::filesystem::path& path, std::uint32_t width,
                           std::uint32_t height) {
    const unsigned char bytes[] = {
        0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,
        0,0,0,13,'I','H','D','R',
        static_cast<unsigned char>(width >> 24), static_cast<unsigned char>(width >> 16),
        static_cast<unsigned char>(width >> 8), static_cast<unsigned char>(width),
        static_cast<unsigned char>(height >> 24), static_cast<unsigned char>(height >> 16),
        static_cast<unsigned char>(height >> 8), static_cast<unsigned char>(height)
    };
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
}

static std::filesystem::path makeSession(std::uint32_t frameCount = 12) {
    const auto root = std::filesystem::temp_directory_path() /
        ("geodan-image-parser-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root / "frames");
    {
        std::ofstream manifest(root / "manifest.json");
        manifest << "{\n"
                 << "\"schema_version\":1,\"status\":\"complete\","
                 << "\"width\":360,\"height\":1080,\"target_fps\":60,"
                 << "\"frames_written\":" << frameCount << ','
                 << "\"missed_capture_intervals\":0,\"writer_queue_drops\":0,"
                 << "\"start_unix_ns\":123,\"end_unix_ns\":456,"
                 << "\"first_sequence\":0,\"last_sequence\":" << (frameCount - 1) << ','
                 << "\"stop_reason\":\"test\",\"frames_metadata\":\"frames.csv\","
                 << "\"frames_directory\":\"frames\"}";
    }
    {
        std::ofstream csv(root / "frames.csv");
        csv << "sequence,timestamp_ns,delta_ns,missed_intervals,write_status\n";
        for (std::uint32_t i = 0; i < frameCount; ++i)
            csv << i << ',' << (static_cast<std::uint64_t>(i) * 16666667ULL) << ','
                << (i == 0 ? 0 : 16666667) << ",0,written\n";
    }
    {
        std::ofstream geometry(root / "hitboxes.csv");
        geometry << "sequence,phase,geometry_status,entity_type,entity_id,left,top,right,bottom\n";
        for (std::uint32_t i = 0; i < frameCount; ++i) {
            geometry << i << ",active_gameplay,valid,frame,,,,,\n";
            geometry << i << ",active_gameplay,valid,player,0,0,0,10,10\n";
            const double hazardLeft = i % 2 == 0 ? 20.0 : 15.0;
            geometry << i << ",active_gameplay,valid,hazard,1," << hazardLeft << ",0,"
                     << (hazardLeft + 10.0) << ",10\n";
        }
    }
    for (std::uint32_t i = 0; i < frameCount; ++i) {
        char name[16];
        std::snprintf(name, sizeof(name), "%06u", i);
        writePngHeader(root / "frames" / (std::string("frame_") + name + ".png"), 360, 1080);
    }
    return root;
}

int main() {
    {
        const auto path = makeSession();
        std::string error;
        const auto result = parseSession(path, error);
        assert(result && result->structurallyValid && result->parserComplete);
        assert(result->sourceFrameCount == 12 && result->processedFrameCount == 12);
        assert(result->sections.size() == 2);
        assert(result->sections[0].firstSampleIndex == 0 && result->sections[0].lastSampleIndex == 9);
        assert(result->sections[1].firstSampleIndex == 10 && result->sections[1].lastSampleIndex == 11);
        assert(result->clearance.meanPx.status == EvidenceStatus::Measured);
        assert(result->clearance.meanPx.sampleCount == 12);
        assert(result->clearance.meanPx.value == 7.5);
        assert(result->sections[0].clearance.minimumPx.value == 5.0);
        assert(result->windows60.status == EvidenceStatus::Missing);
        assert(result->windows120.status == EvidenceStatus::Missing);
        assert(result->windows240.status == EvidenceStatus::Missing);
        assert(result->nerves.status == EvidenceStatus::Missing);
        std::filesystem::remove_all(path);
    }
    {
        const auto path = makeSession(2);
        auto manifest = path / "manifest.json";
        std::ifstream in(manifest);
        std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const auto widthAt = json.find("\"width\":360");
        assert(widthAt != std::string::npos);
        json.replace(widthAt, std::string("\"width\":360").size(), "\"width\":720");
        std::ofstream out(manifest); out << json; out.close();
        for (std::uint32_t i = 0; i < 2; ++i) {
            char name[16]; std::snprintf(name, sizeof(name), "%06u", i);
            writePngHeader(path / "frames" / (std::string("frame_") + name + ".png"), 720, 1080);
        }
        std::string error;
        const auto result = parseSession(path, error);
        assert(result && result->clearance.meanPx.value == 3.75);
        std::filesystem::remove_all(path);
    }
    {
        const auto path = makeSession(1);
        auto geometry = path / "hitboxes.csv";
        std::ifstream in(geometry);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        for (const auto& kind : {"frame", "player", "hazard"}) {
            const auto oldValue = std::string("0,active_gameplay,valid,") + kind;
            const auto newValue = std::string("0,active_gameplay,unsupported,") + kind;
            const auto at = content.find(oldValue);
            assert(at != std::string::npos);
            content.replace(at, oldValue.size(), newValue);
        }
        std::ofstream out(geometry); out << content; out.close();
        std::string error;
        const auto result = parseSession(path, error);
        assert(result && result->unclassifiedFrameCount == 1);
        assert(result->clearance.meanPx.status == EvidenceStatus::Unsupported);
        std::filesystem::remove_all(path);
    }
    {
        const auto path = makeSession(2);
        std::filesystem::remove(path / "hitboxes.csv");
        std::string error;
        const auto result = parseSession(path, error);
        assert(!result && error == "cannot_open_hitboxes_csv");
        std::filesystem::remove_all(path);
    }
    {
        const auto path = makeSession(2);
        auto geometry = path / "hitboxes.csv";
        std::ifstream in(geometry);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        for (const auto& kind : {"frame", "player", "hazard"}) {
            const auto oldValue = std::string("1,active_gameplay,valid,") + kind;
            const auto newValue = std::string("1,active_gameplay,unsupported,") + kind;
            const auto at = content.find(oldValue);
            assert(at != std::string::npos);
            content.replace(at, oldValue.size(), newValue);
        }
        std::ofstream out(geometry);
        out << content;
        out.close();
        std::string error;
        const auto result = parseSession(path, error);
        assert(result && result->structurallyValid && !result->parserComplete);
        assert(result->unclassifiedFrameCount == 1);
        assert(result->sections[0].unsupportedHitboxCount == 1);
        std::filesystem::remove_all(path);
    }
    {
        const auto path = makeSession(2);
        writePngHeader(path / "frames" / "frame_000001.png", 359, 1080);
        std::string error;
        const auto result = parseSession(path, error);
        assert(!result && error == "missing_or_wrong_sized_png");
        std::filesystem::remove_all(path);
    }
    {
        const auto path = makeSession(12);
        auto geometry = path / "hitboxes.csv";
        std::ifstream in(geometry);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        // A phase boundary splits a section before its ten-image capacity.
        for (const auto& kind : {"frame", "player", "hazard"}) {
            const auto oldValue = std::string("5,active_gameplay,valid,") + kind;
            const auto newValue = std::string("5,paused,valid,") + kind;
            const auto at = content.find(oldValue);
            assert(at != std::string::npos);
            content.replace(at, oldValue.size(), newValue);
        }
        std::ofstream out(geometry);
        out << content;
        out.close();
        std::string error;
        const auto result = parseSession(path, error);
        assert(result && result->sections.size() == 3);
        assert(result->sections[0].firstSampleIndex == 0 && result->sections[0].lastSampleIndex == 4);
        assert(result->sections[1].firstSampleIndex == 5 && result->sections[1].lastSampleIndex == 5);
        assert(result->sections[1].phase == Phase::Paused);
        assert(result->sections[2].firstSampleIndex == 6 && result->sections[2].lastSampleIndex == 11);
        std::filesystem::remove_all(path);
    }
    std::cout << "Image Parser tests passed\n";
}
