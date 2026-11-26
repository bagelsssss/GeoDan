// GeoDan-owned hold/release trajectory prediction.
// Uses the game's own player update and collision routines on hidden players.
// Unsupported nearby interactions terminate a path instead of being guessed.
#include "trajectory.hpp"

#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include <algorithm>
#include <array>
#include <cmath>

#include "core/trajectory_model.hpp"

using namespace geode::prelude;
namespace trajectory = geodan::core::trajectory;

namespace {

constexpr std::size_t kPlayerCount = 2;
constexpr std::size_t kBranchCount = 2;
constexpr float kLineWidth = 1.0f;

const ccColor4F kHoldColour = ccc4f(0.18f, 1.0f, 0.30f, 0.95f);
const ccColor4F kReleaseColour = ccc4f(1.0f, 0.22f, 0.18f, 0.95f);

struct TrajectoryRuntime {
    PlayLayer* owner = nullptr;
    std::array<PlayerObject*, kBranchCount> simulationPlayers{};
    CCDrawNode* drawNode = nullptr;
    std::array<trajectory::Prediction, kPlayerCount> predictions{};
    PlayerObject* currentSimulationPlayer = nullptr;
    bool simulationDead = false;
    bool unsafeInteraction = false;
    trajectory::Settings settings{};
};

TrajectoryRuntime* s_runtime = nullptr;

bool isFinitePoint(CCPoint point) {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

bool isMovingObject(GameObject* object) {
    if (!s_runtime || !s_runtime->owner || !s_runtime->owner->m_objectsToMove || !object)
        return false;

    auto* moving = s_runtime->owner->m_objectsToMove;
    for (unsigned int i = 0; i < moving->count(); ++i) {
        if (moving->objectAtIndex(i) == object)
            return true;
    }
    return false;
}

bool isSupportedCollisionObject(GameObject* object) {
    if (!object || object->m_isNoTouch || object->getGroupDisabled())
        return true;
    // The simulator does not advance level movers, so stop before using their
    // current-frame positions as if they were static geometry.
    if (isMovingObject(object))
        return false;

    switch (object->m_objectType) {
        case GameObjectType::Solid:
        case GameObjectType::Hazard:
        case GameObjectType::AnimatedHazard:
        case GameObjectType::Slope:
            return true;
        default:
            return false;
    }
}

void destroyRuntime(TrajectoryRuntime* runtime) {
    if (!runtime) return;

    if (runtime->drawNode) {
        runtime->drawNode->removeFromParentAndCleanup(true);
        runtime->drawNode = nullptr;
    }

    for (auto*& player : runtime->simulationPlayers) {
        if (!player) continue;
        player->removeFromParentAndCleanup(true);
        player->release();
        player = nullptr;
    }

    runtime->currentSimulationPlayer = nullptr;
    if (s_runtime == runtime) s_runtime = nullptr;
    delete runtime;
}

bool initializeRuntime(PlayLayer* layer) {
    if (!layer || !layer->m_objectLayer) return false;

    auto* runtime = new TrajectoryRuntime();
    runtime->owner = layer;

    for (auto*& player : runtime->simulationPlayers) {
        player = PlayerObject::create(1, 1, layer, layer, true);
        if (!player) {
            destroyRuntime(runtime);
            return false;
        }

        player->retain();
        player->setVisible(false);
        player->setOpacity(0);
        layer->m_objectLayer->addChild(player);
    }

    runtime->drawNode = CCDrawNode::create();
    if (!runtime->drawNode) {
        destroyRuntime(runtime);
        return false;
    }

    layer->m_objectLayer->addChild(runtime->drawNode, 10001);
    runtime->drawNode->setVisible(false);
    s_runtime = runtime;
    return true;
}

bool ensureRuntime(PlayLayer* layer) {
    if (s_runtime && s_runtime->owner == layer)
        return s_runtime->drawNode != nullptr;

    if (s_runtime)
        destroyRuntime(s_runtime);

    return initializeRuntime(layer);
}

bool isSimulationPlayer(PlayerObject* player) {
    if (!s_runtime || !player) return false;
    return player == s_runtime->simulationPlayers[0] ||
           player == s_runtime->simulationPlayers[1];
}

void prepareSimulationPlayer(PlayerObject* simulation, PlayerObject* live, bool holdJump) {
    simulation->copyAttributes(live);
    simulation->m_gravityMod = live->m_gravityMod;
    simulation->m_isOnGround = live->m_isOnGround;
    simulation->m_holdingButtons = live->m_holdingButtons;
    simulation->setPosition(live->getPosition());
    simulation->setRotation(live->getRotation());
    simulation->setVisible(false);
    simulation->setOpacity(0);
    // Keep every copied platformer input; only branch the jump button.
    if (holdJump)
        simulation->pushButton(PlayerButton::Jump);
    else
        simulation->releaseButton(PlayerButton::Jump);

    simulation->resetCollisionLog(true);
}

void simulateBranch(TrajectoryRuntime& runtime, PlayLayer* layer, PlayerObject* live,
                    std::size_t playerIndex, trajectory::Branch branch, float frameDt) {
    const auto branchIndex = static_cast<std::size_t>(branch);
    auto* simulation = runtime.simulationPlayers[branchIndex];
    auto& path = runtime.predictions[playerIndex].path(branch);
    path.reset();

    if (!simulation || !live || !isFinitePoint(live->getPosition()) ||
        !std::isfinite(frameDt) || frameDt <= 0.f) {
        path.stop(trajectory::StopReason::InvalidState);
        return;
    }

    prepareSimulationPlayer(simulation, live, branch == trajectory::Branch::Hold);
    const auto start = simulation->getPosition();
    path.append({{start.x, start.y}, simulation->getRotation()},
                runtime.settings.boundedMaxSamples());

    runtime.simulationDead = false;
    runtime.unsafeInteraction = false;
    runtime.currentSimulationPlayer = simulation;

    {
        trajectory::SimulationContext::Scope simulationScope;
        const auto maxSamples = runtime.settings.boundedMaxSamples();

        for (std::size_t step = 1; step < maxSamples; ++step) {
            simulation->resetCollisionLog(true);
            layer->checkCollisions(simulation, frameDt, false);

            if (runtime.simulationDead) {
                path.stop(trajectory::StopReason::Collision);
                break;
            }
            if (runtime.unsafeInteraction) {
                path.stop(trajectory::StopReason::UnsafeInteraction);
                break;
            }

            simulation->update(frameDt);
            simulation->updateRotation(frameDt);

            const auto position = simulation->getPosition();
            const auto rotation = simulation->getRotation();
            if (!isFinitePoint(position) || !std::isfinite(rotation)) {
                path.stop(trajectory::StopReason::InvalidState);
                break;
            }

            if (!path.append({{position.x, position.y}, rotation}, maxSamples))
                break;
        }
    }

    runtime.currentSimulationPlayer = nullptr;
    if (!path.stopped())
        path.stop(trajectory::StopReason::StepLimit);
}

void drawPath(CCDrawNode* node, const trajectory::Path& path, const ccColor4F& colour) {
    if (!node || path.samples.size() < 2) return;

    for (std::size_t i = 1; i < path.samples.size(); ++i) {
        const auto& from = path.samples[i - 1].position;
        const auto& to = path.samples[i].position;
        node->drawSegment(ccp(from.x, from.y), ccp(to.x, to.y), kLineWidth, colour);
    }
}

void drawPredictions(TrajectoryRuntime& runtime) {
    if (!runtime.drawNode) return;
    runtime.drawNode->clear();

    for (const auto& prediction : runtime.predictions) {
        drawPath(runtime.drawNode, prediction.path(trajectory::Branch::Hold), kHoldColour);
        drawPath(runtime.drawNode, prediction.path(trajectory::Branch::Release), kReleaseColour);
    }
}

} // namespace

void updateGeoDanTrajectory(PlayLayer* layer, float dt) {
    if (!layer) return;

    auto* mod = Mod::get();
    if (!mod || !mod->getSettingValue<bool>("trajectory")) {
        if (s_runtime && s_runtime->owner == layer && s_runtime->drawNode) {
            s_runtime->drawNode->clear();
            s_runtime->drawNode->setVisible(false);
        }
        return;
    }

    if (!ensureRuntime(layer)) return;
    auto& runtime = *s_runtime;
    runtime.drawNode->clear();
    runtime.drawNode->setVisible(true);

    const float timeWarp = layer->m_gameState.m_timeWarp;
    if (!std::isfinite(dt) || dt <= 0.f || !std::isfinite(timeWarp) || timeWarp <= 0.f) {
        for (auto& prediction : runtime.predictions) prediction.reset();
        return;
    }

    // Fixed physics step, not frame time: the prediction length must not depend on FPS.
    // timeWarp is still checked above, but does not change the step length.
    constexpr float frameDt = 1.f / 240.f;
    auto* live1 = layer->m_player1;
    auto* live2 = layer->m_player2;

    if (live1 && live1->isVisible()) {
        simulateBranch(runtime, layer, live1, 0, trajectory::Branch::Hold, frameDt);
        simulateBranch(runtime, layer, live1, 0, trajectory::Branch::Release, frameDt);
    } else {
        runtime.predictions[0].reset();
    }

    if (layer->m_gameState.m_isDualMode && live2 && live2->isVisible()) {
        simulateBranch(runtime, layer, live2, 1, trajectory::Branch::Hold, frameDt);
        simulateBranch(runtime, layer, live2, 1, trajectory::Branch::Release, frameDt);
    } else {
        runtime.predictions[1].reset();
    }

    drawPredictions(runtime);
}

void resetGeoDanTrajectory(PlayLayer* layer) {
    if (!s_runtime || s_runtime->owner != layer) return;
    for (auto& prediction : s_runtime->predictions) prediction.reset();
    if (s_runtime->drawNode) s_runtime->drawNode->clear();
}

void cleanupGeoDanTrajectory(PlayLayer* layer) {
    if (s_runtime && s_runtime->owner == layer)
        destroyRuntime(s_runtime);
}

// The simulation must never destroy the live player or end the real level.
class $modify(GeoDanTrajectoryPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (isSimulationPlayer(player) && trajectory::SimulationContext::active()) {
            if (s_runtime) s_runtime->simulationDead = true;
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }

    void playEndAnimationToPos(CCPoint position) {
        if (trajectory::SimulationContext::active()) return;
        PlayLayer::playEndAnimationToPos(position);
    }
};

// Trigger/activation paths are blocked. Candidate collision vectors are copied,
// filtered to ordinary solids/hazards/slopes, then restored after the real checker.
class $modify(GeoDanTrajectoryBaseLayer, GJBaseGameLayer) {
    bool canBeActivatedByPlayer(PlayerObject* player, EffectGameObject* object) {
        if (trajectory::SimulationContext::active()) return false;
        return GJBaseGameLayer::canBeActivatedByPlayer(player, object);
    }

    void flipGravity(PlayerObject* player, bool flip, bool noEffects) {
        if (trajectory::SimulationContext::active()) return;
        GJBaseGameLayer::flipGravity(player, flip, noEffects);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (trajectory::SimulationContext::active()) return;
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }

    void gameEventTriggered(GJGameEvent event, int material, int playerID) {
        if (trajectory::SimulationContext::active()) return;
        GJBaseGameLayer::gameEventTriggered(event, material, playerID);
    }

    cocos2d::CCParticleSystemQuad* spawnParticle(const char* plist, int zOrder,
                                                  cocos2d::tCCPositionType positionType,
                                                  cocos2d::CCPoint position) {
        // Never return nullptr: callers may dereference the result. Spawn normally,
        // then detach the particle again while a simulation is running.
        auto* particle = GJBaseGameLayer::spawnParticle(plist, zOrder, positionType, position);
        if (particle && trajectory::SimulationContext::active())
            particle->removeFromParentAndCleanup(true);
        return particle;
    }

    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* objects,
                               int objectCount, float dt) {
        if (!trajectory::SimulationContext::active() || !s_runtime ||
            player != s_runtime->currentSimulationPlayer || !objects) {
            GJBaseGameLayer::collisionCheckObjects(player, objects, objectCount, dt);
            return;
        }

        gd::vector<GameObject*> original = *objects;
        for (auto* object : original) {
            if (!isSupportedCollisionObject(object)) {
                s_runtime->unsafeInteraction = true;
                break;
            }
        }

        objects->erase(
            std::remove_if(objects->begin(), objects->end(),
                [](GameObject* object) { return !isSupportedCollisionObject(object); }),
            objects->end());

        GJBaseGameLayer::collisionCheckObjects(
            player, objects, static_cast<int>(objects->size()), dt);

        *objects = original;
    }
};

class $modify(GeoDanTrajectoryEffects, EffectGameObject) {
    void triggerObject(GJBaseGameLayer* layer, int uniqueID, gd::vector<int> const* remapKeys) {
        if (trajectory::SimulationContext::active()) return;
        EffectGameObject::triggerObject(layer, uniqueID, remapKeys);
    }
};

class $modify(GeoDanTrajectoryGameObject, GameObject) {
    void playShineEffect() {
        if (trajectory::SimulationContext::active()) return;
        GameObject::playShineEffect();
    }
};

class $modify(GeoDanTrajectoryPlayerObject, PlayerObject) {
    void incrementJumps() {
        if (trajectory::SimulationContext::active()) return;
        PlayerObject::incrementJumps();
    }

    void ringJump(RingObject* object, bool skipCheck) {
        if (trajectory::SimulationContext::active()) return;
        PlayerObject::ringJump(object, skipCheck);
    }

    void playSpiderDashEffect(CCPoint from, CCPoint to) {
        if (trajectory::SimulationContext::active()) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }

    void playBumpEffect(int objectType, GameObject* object) {
        if (trajectory::SimulationContext::active()) return;
        PlayerObject::playBumpEffect(objectType, object);
    }

    void playDeathEffect() {
        if (trajectory::SimulationContext::active()) return;
        PlayerObject::playDeathEffect();
    }

    void playCompleteEffect(bool noEffects, bool instant) {
        if (trajectory::SimulationContext::active()) return;
        PlayerObject::playCompleteEffect(noEffects, instant);
    }

    void playerDestroyed(bool noEffects) {
        if (trajectory::SimulationContext::active()) return;
        PlayerObject::playerDestroyed(noEffects);
    }
};

class $modify(GeoDanTrajectoryHardStreak, HardStreak) {
    void addPoint(CCPoint point) {
        if (trajectory::SimulationContext::active()) return;
        HardStreak::addPoint(point);
    }
};
