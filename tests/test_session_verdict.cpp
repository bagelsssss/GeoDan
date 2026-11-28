// Tests for the I/I Analyzer substage 3 session verdict. Builds synthetic sessions on disk.
#include "core/session_verdict.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>

using namespace geodan::core;
using namespace geodan::core::verdict;
namespace fs = std::filesystem;

namespace {

struct Spec {
    int frames = 200;
    std::string stop = "level_complete";
    int missed = 0;
    int writerDrops = 0;
    bool states = true;
    bool drops = true;
    bool dropsLossy = false;
    int hitchEvery = 0;           // every n-th frame lasts 3 intervals
    int leadingHidden = 20;       // unsupported frames at the start
    int midHidden = 0;            // unsupported frames after the start
    bool incomplete = false;
};

// Player runs at constant speed from frame 20 and never stops, so there is no end screen.
void writeSession(const fs::path& dir, const Spec& spec) {
    fs::create_directories(dir);
    const std::uint64_t interval = 1'000'000'000ull / 60;
    std::vector<std::uint64_t> stamp;
    std::uint64_t t = 0;
    for (int i = 0; i < spec.frames; ++i) {
        const bool hitch = spec.hitchEvery > 0 && i > 0 && i % spec.hitchEvery == 0;
        stamp.push_back(t);
        t += hitch ? 3 * interval : interval;
    }
    {
        std::ofstream m(dir / "manifest.json");
        m << "{\n  \"schema_version\": 2,\n  \"status\": \"" << (spec.incomplete ? "incomplete" : "complete") << "\",\n"
          << "  \"width\": 360,\n  \"height\": 1080,\n  \"target_fps\": 60,\n"
          << "  \"frames_written\": " << spec.frames << ",\n"
          << "  \"missed_capture_intervals\": " << spec.missed << ",\n  \"writer_queue_drops\": " << spec.writerDrops << ",\n"
          << "  \"start_unix_ns\": 1000,\n  \"end_unix_ns\": 2000,\n  \"first_sequence\": 0,\n"
          << "  \"last_sequence\": " << spec.frames - 1 << ",\n  \"stop_reason\": \"" << spec.stop << "\",\n"
          << "  \"frames_metadata\": \"frames.csv\",\n  \"frame_storage\": \"mp4\",\n  \"video_file\": \"video.mp4\",\n"
          << "  \"input_file\": \"inputs.csv\",\n  \"level_name\": \"T\",\n  \"dual_mode\": false,\n  \"frames_directory\": null\n}\n";
    }
    {
        std::ofstream f(dir / "frames.csv");
        f << "sequence,timestamp_ns,delta_ns,missed_intervals,write_status\n";
        for (int i = 0; i < spec.frames; ++i)
            f << i << ',' << stamp[i] << ',' << (i ? stamp[i] - stamp[i - 1] : 0) << ",0,written\n";
    }
    {
        std::ofstream g(dir / "hitboxes.csv");
        g << "sequence,phase,geometry_status,entity_type,entity_id,left,top,right,bottom\n";
        for (int i = 0; i < spec.frames; ++i) {
            const bool hidden = i < spec.leadingHidden || (i >= 100 && i < 100 + spec.midHidden);
            g << i << ",active_gameplay," << (hidden ? "unsupported" : "valid") << ",frame,,,,,\n";
            if (!hidden) {
                g << i << ",active_gameplay,valid,player,1,0,0,10,10\n";
                g << i << ",active_gameplay,valid,hazard,2,13,0,20,10\n";
            }
        }
    }
    { std::ofstream v(dir / "video.mp4", std::ios::binary); v << "not decoded here"; }
    { std::ofstream in(dir / "inputs.csv"); in << "time_ns,down,button,player1\n"; }
    if (spec.states) {
        std::ofstream st(dir / "states.csv");
        st << "sequence,mode,upside_down,gravity,world_x,world_y,mode_p2,world_x_p2,world_y_p2\n";
        for (int i = 0; i < spec.frames; ++i)
            st << i << ",cube,0,0.9," << (i < 20 ? 0.0 : (i - 20) * 5.2) << ",105,,,\n";
    }
    if (spec.drops) {
        std::ofstream d(dir / "capture_drops.csv");
        d << "kind,count\ninputs_dropped," << (spec.dropsLossy ? 2 : 0) << "\ntouches_dropped,0\nno_buffer_drops,0\n";
    }
}

Verdict run(const fs::path& root, const std::string& name, const Spec& spec,
            std::optional<std::uint64_t> video = std::nullopt) {
    writeSession(root / name, spec);
    if (!video && spec.frames > 0) video = static_cast<std::uint64_t>(spec.frames);
    Options o; o.videoFrames = video;
    return verifySession(root / name, o);
}

} // namespace

int main() {
    const fs::path root = fs::path(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp") / "geodan_verdict_test";
    std::error_code ec;
    fs::remove_all(root, ec);

    // A clean finished run is trusted. The start delay is only a note.
    {
        const auto v = run(root, "clean", Spec{});
        assert(v.level == Level::Trusted);
        assert(v.has("start_delay") && v.has("leading_unsupported"));
        assert(v.count(Severity::Degraded) == 0 && v.count(Severity::Rejected) == 0);
    }
    // A run that did not reach the end is degraded.
    assert(run(root, "quit", Spec{.stop = "user_quit"}).has("level_not_finished"));
    assert(run(root, "quit2", Spec{.stop = "level_restart"}).level == Level::Degraded);
    // Capture losses degrade.
    assert(run(root, "missed", Spec{.missed = 3}).has("missed_capture_intervals"));
    assert(run(root, "wdrop", Spec{.writerDrops = 1}).has("writer_queue_drops"));
    assert(run(root, "lossy", Spec{.dropsLossy = true}).has("events_dropped"));
    // An old session without capture_drops.csv is only a note.
    {
        const auto v = run(root, "nodrops", Spec{.drops = false});
        assert(v.has("drops_not_reported") && v.level == Level::Trusted);
    }
    // States missing degrades.
    assert(run(root, "nostates", Spec{.states = false}).has("no_states"));
    // Hitches: a few are a note, many degrade.
    {
        const auto few = run(root, "hitchfew", Spec{.hitchEvery = 100});
        assert(few.has("frame_hitches") && few.level == Level::Trusted);
        const auto many = run(root, "hitchmany", Spec{.hitchEvery = 10});
        assert(many.level == Level::Degraded);
    }
    // Unsupported frames after the start: a few are a note, many degrade.
    {
        const auto few = run(root, "hidefew", Spec{.midHidden = 5});
        assert(few.has("unsupported_frames") && few.level == Level::Trusted);
        const auto many = run(root, "hidemany", Spec{.midHidden = 40});
        assert(many.level == Level::Degraded);
    }
    // Video check: a frame-count mismatch rejects; no count is a note.
    {
        assert(run(root, "vmismatch", Spec{}, 199).level == Level::Rejected);
        writeSession(root / "vnone", Spec{});
        const auto v = verifySession(root / "vnone");
        assert(v.has("video_not_checked") && v.level == Level::Trusted);
    }
    // A session the analyzer refuses is rejected with its reason.
    {
        const auto v = run(root, "incomplete", Spec{.incomplete = true});
        assert(v.level == Level::Rejected && v.has("session_not_complete"));
        const auto missing = verifySession(root / "does_not_exist");
        assert(missing.level == Level::Rejected);
    }
    // Rejected outranks degraded.
    {
        const auto v = run(root, "both", Spec{.stop = "user_quit"}, 150);
        assert(v.level == Level::Rejected && v.has("level_not_finished") && v.has("video_frame_mismatch"));
    }

    std::puts("Session verdict tests passed");
    return 0;
}
