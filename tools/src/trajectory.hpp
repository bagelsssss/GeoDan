// GeoDan Tools - trajectory overlay (game side). Predicts the hold and release
// paths with a hidden copy of the player that runs the game's own movement and
// collision code. The copy never changes the real level.
#pragma once

#include <Geode/Geode.hpp>

namespace geodan::tools::trajectory {

// Once per physics tick while the trajectory setting is on.
void update(geode::prelude::PlayLayer* layer);

// Draws the latest paths into a node that shares the object layer's space.
void draw(geode::prelude::CCDrawNode* node);

// Level reset, or the setting is turned off.
void reset();

}  // namespace geodan::tools::trajectory
