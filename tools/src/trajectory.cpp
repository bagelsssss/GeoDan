// GeoDan Tools - trajectory, game side. Rules live in core/trajectory_model.hpp.
// Every game call here is checked against the Geode 5.11.2 bindings for
// GD 2.2081 (PlayerObject, PlayLayer, GJBaseGameLayer, GameObject, ...).
#include <Geode/Geode.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

#include "core/trajectory_model.hpp"
#include "trajectory.hpp"

using namespace geode::prelude;
namespace model = geodan::core::trajectory;

namespace {

// The hidden copy's state, read by the hooks below.
struct Session {
    PlayLayer* layer = nullptr;
    CCLayer* copyLayer = nullptr;             // hidden, under the object layer
    std::array<PlayerObject*, 2> copies{};    // one copy per real player
    bool running = false;                     // a branch is being simulated
    bool halted = false;                      // the branch must stop now
    model::StopReason halt = model::StopReason::None;
    int haltObject = 0;
    std::size_t pointCount = 0;               // points on the current branch
    std::size_t guessFrom = model::kNoIndex;
    int guessObject = 0;
    std::size_t triggers = 0;
    std::uint64_t tick = 0;
    std::array<model::InputState, 2> lastInput{};
    std::array<std::array<model::Branch, 2>, 2> paths{};   // [player][hold, release]
    std::string lastLog;
};
Session g;

bool isCopy(PlayerObject* p) {
    return p && (p == g.copies[0] || p == g.copies[1]);
}

std::uint32_t modeBits(PlayerObject* p) {
    std::uint32_t bits = 0;
    if (p->m_isShip) bits |= 1u << 0;
    if (p->m_isBird) bits |= 1u << 1;
    if (p->m_isBall) bits |= 1u << 2;
    if (p->m_isDart) bits |= 1u << 3;
    if (p->m_isRobot) bits |= 1u << 4;
    if (p->m_isSpider) bits |= 1u << 5;
    if (p->m_isSwing) bits |= 1u << 6;
    return bits;
}

model::InputState inputOf(PlayerObject* p) {
    model::InputState s;
    if (!p) return s;
    s.modeBits = modeBits(p);
    s.holding = static_cast<bool>(p->m_holdingButtons[static_cast<int>(PlayerButton::Jump)]);
    s.grounded = p->m_isOnGround;
    s.grounded2 = p->m_isOnGround2;
    s.upsideDown = p->m_isUpsideDown;
    s.speed = p->m_playerSpeed;
    s.gravity = p->m_gravityMod;
    return s;
}

void dropCopies() {
    g.running = false;
    g.halted = false;
    for (auto*& c : g.copies) {
        if (!c) continue;
        c->removeFromParentAndCleanup(true);
        c->release();
        c = nullptr;
    }
    if (g.copyLayer) {
        g.copyLayer->removeAllChildrenWithCleanup(true);
        g.copyLayer->removeFromParentAndCleanup(true);
        g.copyLayer->release();
        g.copyLayer = nullptr;
    }
}

// Creates the hidden layer and copies when missing. True when anything was made.
bool ensureCopies(PlayLayer* layer) {
    bool made = false;
    if (!g.copyLayer) {
        g.copyLayer = CCLayer::create();
        if (!g.copyLayer) return false;
        g.copyLayer->retain();
        g.copyLayer->setVisible(false);
        layer->m_objectLayer->addChild(g.copyLayer, 10002);
        made = true;
    }
    for (auto*& c : g.copies) {
        if (c) continue;
        c = PlayerObject::create(1, 1, layer, g.copyLayer, true);
        if (!c) continue;
        c->retain();
        c->setVisible(false);
        g.copyLayer->addChild(c);
        made = true;
    }
    return made;
}

// Copies the real player's state so the copy starts from the same moment.
void syncCopy(PlayerObject* copy, PlayerObject* real) {
    copy->setVisible(false);
    copy->resetTouchedRings(true);
    copy->copyAttributes(real);
    copy->setScale(real->getScale());
    copy->setPosition(real->getPosition());
    copy->setRotation(real->getRotation());
    copy->m_gravityMod = real->m_gravityMod;
    copy->m_yVelocity = real->m_yVelocity;
    copy->m_platformerXVelocity = real->m_platformerXVelocity;
    copy->m_isPlatformer = real->m_isPlatformer;
    copy->m_platformerMovingLeft = real->m_platformerMovingLeft;
    copy->m_platformerMovingRight = real->m_platformerMovingRight;
    copy->m_isOnGround = real->m_isOnGround;
    copy->m_isOnGround2 = real->m_isOnGround2;
    copy->m_isUpsideDown = real->m_isUpsideDown;
    copy->m_playerSpeed = real->m_playerSpeed;
    copy->m_holdingButtons = real->m_holdingButtons;
    copy->m_jumpBuffered = real->m_jumpBuffered;
    copy->m_wasJumpBuffered = real->m_wasJumpBuffered;
    copy->m_stateJumpBuffered = real->m_stateJumpBuffered;
    copy->m_stateRingJump = real->m_stateRingJump;
    copy->m_stateRingJump2 = real->m_stateRingJump2;
    copy->m_wasRobotJump = real->m_wasRobotJump;
}

void clearCollisionLogs(PlayerObject* p) {
    if (p->m_collisionLogTop) p->m_collisionLogTop->removeAllObjects();
    if (p->m_collisionLogBottom) p->m_collisionLogBottom->removeAllObjects();
    if (p->m_collisionLogLeft) p->m_collisionLogLeft->removeAllObjects();
    if (p->m_collisionLogRight) p->m_collisionLogRight->removeAllObjects();
}

model::Point inObjectSpace(PlayerObject* p, CCNode* objectLayer) {
    auto* parent = p->getParent();
    const auto world = parent ? parent->convertToWorldSpace(p->getPosition()) : p->getPosition();
    const auto local = objectLayer->convertToNodeSpace(world);
    return {local.x, local.y};
}

// Objects the game's collision code should see during a prediction.
bool isSolidLike(GameObject* o) {
    switch (o->getType()) {
        case GameObjectType::Solid:
        case GameObjectType::Hazard:
        case GameObjectType::Slope:
        case GameObjectType::CollisionObject:
        case GameObjectType::AnimatedHazard:
            return true;
        default:
            return false;
    }
}

// Pads, orbs, portals and speed portals: their effects are not modelled.
bool isTrigger(GameObject* o) {
    switch (o->m_objectID) {
        case 200: case 201: case 202: case 203: case 1334:  // speed portals
            return true;
        default:
            break;
    }
    switch (o->getType()) {
        case GameObjectType::YellowJumpPad:
        case GameObjectType::PinkJumpPad:
        case GameObjectType::RedJumpPad:
        case GameObjectType::GravityPad:
        case GameObjectType::SpiderPad:
        case GameObjectType::YellowJumpRing:
        case GameObjectType::PinkJumpRing:
        case GameObjectType::RedJumpRing:
        case GameObjectType::GravityRing:
        case GameObjectType::GreenRing:
        case GameObjectType::DropRing:
        case GameObjectType::CustomRing:
        case GameObjectType::DashRing:
        case GameObjectType::GravityDashRing:
        case GameObjectType::SpiderOrb:
        case GameObjectType::InverseGravityPortal:
        case GameObjectType::NormalGravityPortal:
        case GameObjectType::GravityTogglePortal:
        case GameObjectType::ShipPortal:
        case GameObjectType::CubePortal:
        case GameObjectType::BallPortal:
        case GameObjectType::UfoPortal:
        case GameObjectType::WavePortal:
        case GameObjectType::RobotPortal:
        case GameObjectType::SpiderPortal:
        case GameObjectType::SwingPortal:
        case GameObjectType::DualPortal:
        case GameObjectType::SoloPortal:
        case GameObjectType::RegularSizePortal:
        case GameObjectType::MiniSizePortal:
        case GameObjectType::InverseMirrorPortal:
        case GameObjectType::NormalMirrorPortal:
        case GameObjectType::TeleportPortal:
        case GameObjectType::TeleportOrb:
            return true;
        default:
            return false;
    }
}

void haltBranch(model::StopReason reason, int object) {
    if (!g.running || g.halted) return;  // the first reason wins
    g.halted = true;
    g.halt = reason;
    g.haltObject = object;
}

model::Branch simulate(PlayLayer* layer, PlayerObject* real, PlayerObject* copy,
                       bool holding, std::size_t steps) {
    model::Branch out;
    out.requestedSteps = steps;
    if (!layer || !real || !copy || real->m_isDead) {
        out.stop = model::StopReason::PlayerUnavailable;
        return out;
    }

    syncCopy(copy, real);
    out.points.push_back(inObjectSpace(real, layer->m_objectLayer));

    // Set the copy's button, then start the branch with its hooks active.
    const bool nowHolding = static_cast<bool>(real->m_holdingButtons[static_cast<int>(PlayerButton::Jump)]);
    if (holding != nowHolding) {
        if (holding) copy->pushButton(PlayerButton::Jump);
        else copy->releaseButton(PlayerButton::Jump);
    }

    g.running = true;
    g.halted = false;
    g.halt = model::StopReason::None;
    g.haltObject = 0;
    g.pointCount = out.points.size();
    g.guessFrom = model::kNoIndex;
    g.guessObject = 0;
    g.triggers = 0;

    bool stopped = false;
    for (std::size_t i = 0; i < steps; ++i) {
        // The game's own order: collide at the current position, then move.
        clearCollisionLogs(copy);
        layer->checkCollisions(copy, model::kStepSeconds, false);
        if (copy->m_isDead) {
            out.stop = model::StopReason::HazardDeath;
            out.stopObjectID = g.haltObject;
            stopped = true;
            break;
        }
        copy->update(model::kStepSeconds);
        copy->updateRotation(model::kStepSeconds);

        const auto p = inObjectSpace(copy, layer->m_objectLayer);
        if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
            out.stop = model::StopReason::InvalidPosition;
            stopped = true;
            break;
        }
        out.points.push_back(p);
        g.pointCount = out.points.size();
        out.completedSteps = i + 1;

        if (g.halted) {
            out.stop = g.halt;
            out.stopObjectID = g.haltObject;
            stopped = true;
            break;
        }
    }
    if (!stopped) out.stop = model::StopReason::HorizonReached;

    out.guessFrom = g.guessFrom;
    out.guessObjectID = g.guessObject;
    out.triggersPassed = g.triggers;

    g.running = false;
    g.halted = false;
    return out;
}

void writeLog(const std::string& line) {
    if (line == g.lastLog) return;
    g.lastLog = line;
    // Capped at 1 MB so a long session cannot fill the disk.
    std::error_code ec;
    const auto path = Mod::get()->getSaveDir() / "trajectory.log";
    if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) >= 1000000) return;
    if (FILE* f = std::fopen(path.string().c_str(), "a")) {
        std::fprintf(f, "%s\n", line.c_str());
        std::fclose(f);
    }
}

void drawBranch(CCDrawNode* node, const model::Branch& b, ccColor4F colour) {
    if (b.points.size() < 2) return;
    for (std::size_t i = 1; i < b.points.size(); ++i) {
        ccColor4F c = colour;
        // Segments after an unmodelled trigger are a guess: faded.
        if (b.guessFrom != model::kNoIndex && i - 1 >= b.guessFrom) c.a *= 0.35f;
        node->drawSegment(ccp(b.points[i - 1].x, b.points[i - 1].y),
                          ccp(b.points[i].x, b.points[i].y), 1.5f, c);
    }
}

}  // namespace

namespace geodan::tools::trajectory {

void reset() {
    dropCopies();
    g.layer = nullptr;
    g.tick = 0;
    g.lastInput = {};
    g.paths = {};
    g.lastLog.clear();
}

void update(PlayLayer* layer) {
    if (!layer || !layer->m_player1 || !layer->m_objectLayer) return;
    if (g.layer != layer) {
        reset();
        g.layer = layer;
    }

    const bool made = ensureCopies(layer);
    const std::array<model::InputState, 2> input = {
        inputOf(layer->m_player1), inputOf(layer->m_player2)};
    ++g.tick;
    const bool changed = !(input[0] == g.lastInput[0]) || !(input[1] == g.lastInput[1]);
    if (!model::needsRefresh(made, changed, g.tick) || !g.copies[0]) return;

    const std::size_t steps = model::kDefaultSteps;
    g.paths[0] = {simulate(layer, layer->m_player1, g.copies[0], true, steps),
                  simulate(layer, layer->m_player1, g.copies[0], false, steps)};
    if (layer->m_player2 && g.copies[1]) {
        g.paths[1] = {simulate(layer, layer->m_player2, g.copies[1], true, steps),
                      simulate(layer, layer->m_player2, g.copies[1], false, steps)};
    }
    g.lastInput = input;

    writeLog("trajectory P1 " + model::describe("hold", g.paths[0][0]) + " | " +
             model::describe("release", g.paths[0][1]));
}

void draw(CCDrawNode* node) {
    if (!node) return;
    const ccColor4F hold = ccc4f(0.02f, 0.96f, 0.02f, 0.96f);
    const ccColor4F release = ccc4f(0.00f, 0.62f, 0.00f, 0.90f);
    for (const auto& player : g.paths) {
        drawBranch(node, player[0], hold);
        drawBranch(node, player[1], release);
    }
}

}  // namespace geodan::tools::trajectory

// Hooks that keep the hidden copy from changing the real level. Each one acts
// only while a branch is running for a copy.

class $modify(GeoDanTrajectoryBase, GJBaseGameLayer) {
    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* objects,
                               int count, float dt) {
        if (!isCopy(player) || !g.running) {
            GJBaseGameLayer::collisionCheckObjects(player, objects, count, dt);
            return;
        }
        // Only solid-like objects reach the game's collision code. Triggers are
        // noted (the path continues past them, marked as a guess).
        static gd::vector<GameObject*> solid;
        solid.clear();
        const auto rect = player->getObjectRect();
        if (objects) {
            for (int i = 0; i < count; ++i) {
                auto* o = (*objects)[i];
                if (!o) continue;
                if (isSolidLike(o)) {
                    solid.push_back(o);
                } else if (isTrigger(o) && rect.intersectsRect(o->getObjectRect())) {
                    if (g.guessFrom == model::kNoIndex) {
                        g.guessFrom = g.pointCount == 0 ? 0 : g.pointCount - 1;
                        g.guessObject = o->m_objectID;
                    }
                    ++g.triggers;
                }
            }
        }
        GJBaseGameLayer::collisionCheckObjects(player, &solid, static_cast<int>(solid.size()), dt);
    }

    void gameEventTriggered(GJGameEvent event, int material, int playerID) {
        if (g.running) return;
        GJBaseGameLayer::gameEventTriggered(event, material, playerID);
    }

    void flipGravity(PlayerObject* player, bool flip, bool noEffects) {
        if (isCopy(player) && g.running) {
            player->flipGravity(flip, true);
            return;
        }
        GJBaseGameLayer::flipGravity(player, flip, noEffects);
    }

    bool canBeActivatedByPlayer(PlayerObject* player, EffectGameObject* effect) {
        if (g.running) return false;
        return GJBaseGameLayer::canBeActivatedByPlayer(player, effect);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (g.running) return;
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }

    void playerWillSwitchMode(PlayerObject* player, GameObject* object) {
        if (isCopy(player) && g.running) {
            haltBranch(model::StopReason::ModeSwitch, object ? object->m_objectID : 0);
            return;
        }
        GJBaseGameLayer::playerWillSwitchMode(player, object);
    }
};

class $modify(GeoDanTrajectoryPlay, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* cause) {
        // A copy that dies stops its branch. The real level's death flow never runs.
        if (isCopy(player) && g.running) {
            haltBranch(model::StopReason::HazardDeath, cause ? cause->m_objectID : 0);
            return;
        }
        PlayLayer::destroyPlayer(player, cause);
    }
};

class $modify(GeoDanTrajectoryStreak, HardStreak) {
    void addPoint(CCPoint point) {
        if (g.running) return;
        HardStreak::addPoint(point);
    }
};

class $modify(GeoDanTrajectoryEffect, EffectGameObject) {
    void triggerObject(GJBaseGameLayer* layer, int uniqueID, gd::vector<int> const* remapKeys) {
        if (g.running) return;
        EffectGameObject::triggerObject(layer, uniqueID, remapKeys);
    }
};

class $modify(GeoDanTrajectoryObject, GameObject) {
    void playShineEffect() {
        if (g.running) return;
        GameObject::playShineEffect();
    }
};

class $modify(GeoDanTrajectoryPlayer, PlayerObject) {
    void ringJump(RingObject* ring, bool skipCheck) {
        if (isCopy(this) && g.running) return;
        PlayerObject::ringJump(ring, skipCheck);
    }

    void incrementJumps() {
        if (isCopy(this) && g.running) return;
        PlayerObject::incrementJumps();
    }

    void playSpiderDashEffect(CCPoint from, CCPoint to) {
        if (isCopy(this) && g.running) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }
};
