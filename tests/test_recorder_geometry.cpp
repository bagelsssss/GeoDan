#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

#include "recorder_geometry.hpp"

namespace rec = geodan::core::recorder;

int main() {
    const rec::CaptureTransform transform{360, 1080, 1.0, 2.0, {100.0, 200.0}};
    if (!transform.valid()) return 1;

    const auto center = transform.worldToCapture({100.0, 200.0});
    if (center.x != 180.0 || center.y != 540.0) return 2;

    const auto rightUp = transform.worldToCapture({110.0, 210.0});
    if (rightUp.x != 200.0 || rightUp.y != 520.0) return 3;

    const std::vector<rec::CapturePoint> corners{
        {95.0, 195.0}, {105.0, 195.0}, {105.0, 205.0}, {95.0, 205.0}
    };
    const auto bounds = rec::boundsFromWorldPoints(corners, transform);
    if (!bounds.valid() || bounds.left != 170.0 || bounds.right != 190.0 ||
        bounds.top != 530.0 || bounds.bottom != 550.0) return 4;

    const auto clipped = rec::clipToCapture({-5.0, 10.0, 20.0, 30.0}, 360, 1080);
    if (!clipped.valid() || clipped.left != 0.0 || clipped.right != 20.0) return 5;
    if (rec::intersectsCapture({-20.0, 10.0, 5.0, 20.0}, 360, 1080) == false) return 6;
    if (rec::intersectsCapture({-20.0, 10.0, -1.0, 20.0}, 360, 1080)) return 7;

    rec::FrameGeometry frame;
    frame.sequence = 17;
    frame.phase = rec::CapturePhase::ActiveGameplay;
    frame.status = rec::GeometryStatus::Valid;
    frame.entities.push_back({rec::GeometryEntityKind::Player, 1, {170, 530, 190, 550}});
    frame.entities.push_back({rec::GeometryEntityKind::Hazard, 42, {200, 530, 220, 550}});
    std::ostringstream csv;
    if (!rec::writeGeometryCsvFrame(csv, frame)) return 8;
    const auto text = csv.str();
    if (text.find("17,active_gameplay,valid,frame,,,,,") != 0) return 9;
    if (text.find("17,active_gameplay,valid,player,1,170,530,190,550") == std::string::npos) return 10;

    frame.entities.push_back({rec::GeometryEntityKind::Hazard, 42, {230, 530, 240, 550}});
    if (frame.valid()) return 11; // duplicate type/id in one frame
    frame.entities.back().id = 43;
    frame.entities.back().bounds = {10, 10, 10, 20};
    if (frame.valid()) return 12; // zero-width rectangle
    frame.entities.clear();
    if (frame.valid()) return 13; // valid geometry requires a player
    if (rec::CaptureTransform{0, 1080, 1.0, 2.0, {0,0}}.valid()) return 14;

    std::cout << "recorder geometry: all checks passed\n";
    return 0;
}
