#pragma once

#include <Geode/Geode.hpp>

// Called from the active PlayLayer's postUpdate/reset/quit hooks.
void updateGeoDanTrajectory(PlayLayer* layer, float dt);
void resetGeoDanTrajectory(PlayLayer* layer);
void cleanupGeoDanTrajectory(PlayLayer* layer);
