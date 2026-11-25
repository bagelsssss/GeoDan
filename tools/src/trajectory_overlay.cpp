// GeoDan trajectory overlay. This renders two isolated first-pass predictions;
// it never advances or changes the real PlayerObject.
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>
#include <vector>

#include "core/trajectory_model.hpp"

using namespace geode::prelude;
namespace tr = geodan::core::trajectory;

namespace {

const ccColor4F kHoldColour = ccc4f(0.20f, 0.72f, 1.00f, 0.90f);
const ccColor4F kReleaseColour = ccc4f(1.00f, 0.72f, 0.18f, 0.90f);

bool makeState(PlayerObject* player, tr::State& out) {
    if (!player || player->m_isDead || player->m_isLocked) return false;

    const auto pos = player->getPosition();
    const auto rect = player->getObjectRect();
    out.position = {pos.x, pos.y};
    out.velocityY = static_cast<float>(player->m_yVelocity);
    out.velocityX = static_cast<float>(player->m_platformerXVelocity);
    out.gravity = player->m_gravity;
    out.gravityMultiplier = player->m_gravityMod;
    out.jumpVelocity = player->m_yStart;
    out.playerSpeed = player->m_playerSpeed;
    out.speedMultiplier = player->m_speedMultiplier;
    out.scale = player->m_playerScale;
    out.halfWidth = (rect.getMaxX() - rect.getMinX()) * 0.5f;
    out.halfHeight = (rect.getMaxY() - rect.getMinY()) * 0.5f;
    out.upsideDown = player->m_isUpsideDown;
    out.grounded = player->m_onGround;
    out.canJump = player->m_canJump;
    out.jumpUnused = player->m_isJumpUnused;
    out.jumping = player->m_isJumping;
    out.holdingJump = player->m_holdingJump;
    out.isPlatformer = player->m_isPlatformer;
    out.goingLeft = player->m_isGoingLeft;

    if (player->m_isShip) out.mode = tr::Mode::Ship;
    else if (player->m_isBird) out.mode = tr::Mode::Ufo;
    else if (player->m_isDart) out.mode = tr::Mode::Wave;
    else if (player->m_isSwing) out.mode = tr::Mode::Swing;
    else if (player->m_isBall) out.mode = tr::Mode::Ball;
    else if (player->m_isRobot) out.mode = tr::Mode::Robot;
    else if (player->m_isSpider) out.mode = tr::Mode::Spider;
    else out.mode = tr::Mode::Cube;

    // The first pass does not model rotated gravity axes or platformer input.
    if (player->m_isSideways) out.mode = tr::Mode::Unsupported;
    return true;
}

std::vector<tr::Obstacle> collectObstacles(PlayLayer* layer) {
    std::vector<tr::Obstacle> result;
    if (!layer || !layer->m_objects) return result;
    result.reserve(layer->m_objects->count());

    for (unsigned int i = 0; i < layer->m_objects->count(); ++i) {
        auto* object = static_cast<GameObject*>(layer->m_objects->objectAtIndex(i));
        if (!object || object->getGroupDisabled() || object->m_isNoTouch) continue;

        const int type = static_cast<int>(object->getType());
        tr::ObjectKind kind;
        if (type == 25) {
            kind = tr::ObjectKind::Slope;
        } else if (type == 2 || type == 47) {
            kind = tr::ObjectKind::Hazard;
        } else if (type == 0 || type == 21) {
            kind = tr::ObjectKind::Solid;
        } else {
            // Decorations are not collision candidates. All other object
            // classes are treated as interactions and terminate prediction
            // if the path reaches them.
            if (type == 7) continue;
            kind = tr::ObjectKind::Interaction;
        }

        const auto rect = object->getObjectRect();
        result.push_back({{rect.getMinX(), rect.getMinY(), rect.getMaxX(), rect.getMaxY()}, kind});
    }
    return result;
}

void drawPath(CCDrawNode* node, const tr::Result& path, const ccColor4F& colour) {
    if (!node || path.samples.size() < 2) return;
    const float count = static_cast<float>(path.samples.size() - 1);
    for (std::size_t i = 1; i < path.samples.size(); ++i) {
        const auto& a = path.samples[i - 1].position;
        const auto& b = path.samples[i].position;
        const float fade = 1.f - 0.65f * (static_cast<float>(i) / count);
        const auto c = ccc4f(colour.r * fade, colour.g * fade, colour.b * fade,
                             colour.a * fade);
        node->drawSegment(ccp(a.x, a.y), ccp(b.x, b.y), 1.8f, c);
    }
}

} // namespace

class $modify(GeoDanTrajectoryPlayLayer, PlayLayer) {
    struct Fields {
        CCDrawNode* trajectoryNode = nullptr;
    };

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto* mod = Mod::get();
        const bool enabled = mod->getSettingValue<bool>("trajectory");
        if (!m_objectLayer) return;

        auto& fields = *m_fields.self();
        if (!fields.trajectoryNode) {
            fields.trajectoryNode = CCDrawNode::create();
            if (!fields.trajectoryNode) return;
            m_objectLayer->addChild(fields.trajectoryNode, 10001);
        }

        fields.trajectoryNode->clear();
        fields.trajectoryNode->setVisible(enabled);
        if (!enabled) return;

        tr::State state;
        if (!makeState(m_player1, state)) return;
        const auto obstacles = collectObstacles(this);
        const auto branches = tr::predictBoth(state, obstacles);
        drawPath(fields.trajectoryNode, branches.hold, kHoldColour);
        drawPath(fields.trajectoryNode, branches.release, kReleaseColour);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (m_fields->trajectoryNode) m_fields->trajectoryNode->clear();
    }
};
