// GeoDan Tools - Geode layer. Draws the TOOLS popup and reads and writes
// settings. The tool list and its rules live in core/tool_registry.hpp.
#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/Popup.hpp>

#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include "core/tool_registry.hpp"

using namespace geode::prelude;
namespace core = geodan::core;

namespace {

constexpr int kToolsButtonTag = 0x6744;
// Side panel size. Sized to fit the game window (about 320 points tall).
constexpr float kWidth = 290.f;
constexpr float kHeight = 300.f;
constexpr float kRowHeight = 32.f;
constexpr float kHeaderHeight = 16.f;
constexpr float kPillWidth = 40.f;
constexpr float kPillHeight = 22.f;
constexpr float kEdgeGap = 10.f;

// Saved value, or the default from mod.json when nothing is saved yet.
bool savedOrDefault(std::string_view id, bool) {
    return Mod::get()->getSettingValue<bool>(std::string(id));
}

// Which popup section a tool belongs to.
enum class Section { Hitboxes, Trajectory, Planned };

Section sectionOf(const core::ToolDef& tool) {
    if (!core::canEnable(tool)) return Section::Planned;
    if (tool.id == "trajectory") return Section::Trajectory;
    return Section::Hitboxes;
}

// Points on a circle arc from angle a0 to a1 (radians), centred on c.
void appendArc(std::vector<CCPoint>& pts, CCPoint c, float r, float a0, float a1) {
    constexpr int steps = 12;
    for (int i = 0; i <= steps; ++i) {
        const float a = a0 + (a1 - a0) * static_cast<float>(i) / steps;
        pts.push_back(ccp(c.x + r * std::cos(a), c.y + r * std::sin(a)));
    }
}

// A pill switch: a rounded track with a white knob on the right when on.
// Built from draw nodes so it does not depend on game sprite names.
CCNode* makePill(bool on, bool enabled) {
    auto* node = CCNode::create();
    node->setContentSize(CCSize(kPillWidth, kPillHeight));
    auto* draw = CCDrawNode::create();
    const float r = kPillHeight / 2.f;
    const float knobX = kPillWidth / 2.f - r;
    std::vector<CCPoint> track;
    appendArc(track, ccp(-knobX, 0.f), r, M_PI / 2.f, 3.f * M_PI / 2.f);
    appendArc(track, ccp(knobX, 0.f), r, -M_PI / 2.f, M_PI / 2.f);
    ccColor4F fill = !enabled ? ccc4f(0.22f, 0.22f, 0.25f, 1.f)
                   : on ? ccc4f(0.25f, 0.72f, 0.40f, 1.f)
                        : ccc4f(0.40f, 0.40f, 0.46f, 1.f);
    draw->drawPolygon(track.data(), static_cast<unsigned int>(track.size()), fill,
                      0.f, ccc4f(0, 0, 0, 0));
    std::vector<CCPoint> knob;
    appendArc(knob, ccp(on ? knobX : -knobX, 0.f), r - 4.f, 0.f, 2.f * M_PI);
    draw->drawPolygon(knob.data(), static_cast<unsigned int>(knob.size()),
                      ccc4f(0.96f, 0.96f, 0.96f, enabled ? 1.f : 0.5f), 0.f, ccc4f(0, 0, 0, 0));
    draw->setPosition(ccp(kPillWidth / 2.f, kPillHeight / 2.f));
    node->addChild(draw);
    return node;
}

class ToolsPopup : public geode::Popup {
    CCNode* m_content = nullptr;

public:
    static ToolsPopup* create() {
        auto* popup = new ToolsPopup();
        if (popup && popup->initPopup()) {
            popup->autorelease();
            return popup;
        }
        CC_SAFE_DELETE(popup);
        return nullptr;
    }

private:
    bool initPopup() {
        if (!Popup::init(kWidth, kHeight)) return false;
        this->setTitle("GeoDan Tools", "goldFont.fnt", 0.55f, 14.f);
        this->rebuild();
        // Slide the panel to the right edge; the dim backdrop still covers the screen.
        const auto win = CCDirector::get()->getWinSize();
        m_mainLayer->setPosition(ccp(win.width - kWidth / 2.f - kEdgeGap, win.height / 2.f));
        return true;
    }

    // Lays the popup out from the top down: one header per section, then its
    // rows. Rebuilt after every change so the pills match the settings.
    void rebuild() {
        if (m_content) m_content->removeFromParent();
        m_content = CCNode::create();
        m_mainLayer->addChild(m_content, 5);

        const auto size = m_mainLayer->getContentSize();
        float y = size.height - 42.f;

        y = addSection(y, "HITBOXES", Section::Hitboxes, size);
        y = addSection(y, "TRAJECTORY", Section::Trajectory, size);
        y = addSection(y, "COMING NEXT", Section::Planned, size);

        auto* summary = CCLabelBMFont::create(
            ("Tools on: " + std::to_string(core::countOn(savedOrDefault))).c_str(), "chatFont.fnt");
        summary->setScale(0.5f);
        summary->setColor(ccc3(190, 190, 190));
        summary->setPosition(ccp(size.width / 2.f, 12.f));
        m_content->addChild(summary);
    }

    float addSection(float y, const char* header, Section section, CCSize size) {
        auto* label = CCLabelBMFont::create(header, "goldFont.fnt");
        label->setScale(0.4f);
        label->setAnchorPoint(ccp(0.f, 0.5f));
        label->setPosition(ccp(16.f, y - kHeaderHeight / 2.f));
        m_content->addChild(label);
        y -= kHeaderHeight;

        auto* menu = CCMenu::create();
        menu->setPosition(ccp(0.f, 0.f));
        menu->setContentSize(size);
        m_content->addChild(menu);

        for (std::size_t i = 0; i < core::kToolCount; ++i) {
            const auto& tool = core::kTools[i];
            if (sectionOf(tool) != section) continue;
            addRow(menu, i, tool, y, size);
            y -= kRowHeight;
        }
        return y - 4.f;
    }

    void addRow(CCMenu* menu, std::size_t index, const core::ToolDef& tool,
                float y, CCSize size) {
        const bool available = core::canEnable(tool);
        const bool on = core::isOn(tool, savedOrDefault);

        auto* title = CCLabelBMFont::create(std::string(tool.title).c_str(), "bigFont.fnt");
        title->setScale(0.28f);
        title->setAnchorPoint(ccp(0.f, 0.5f));
        title->setPosition(ccp(16.f, y - kRowHeight / 2.f + 6.f));
        title->setColor(available ? ccc3(255, 255, 255) : ccc3(120, 120, 120));
        m_content->addChild(title);

        auto* blurb = CCLabelBMFont::create(std::string(tool.blurb).c_str(), "chatFont.fnt");
        blurb->setScale(0.34f);
        blurb->setAnchorPoint(ccp(0.f, 0.5f));
        blurb->setPosition(ccp(16.f, y - kRowHeight / 2.f - 6.f));
        blurb->setColor(ccc3(165, 165, 165));
        m_content->addChild(blurb);

        // Pill switch on the right. Planned tools show a greyed, disabled pill.
        auto* pill = CCMenuItemSpriteExtra::create(
            makePill(on, available), this, menu_selector(ToolsPopup::onToggle));
        pill->setTag(static_cast<int>(index));
        pill->setEnabled(available);
        pill->setPosition(ccp(size.width - 16.f - kPillWidth / 2.f,
                              y - kRowHeight / 2.f));
        menu->addChild(pill);
    }

    void onToggle(CCObject* sender) {
        const auto index = static_cast<std::size_t>(static_cast<CCNode*>(sender)->getTag());
        if (index >= core::kToolCount) return;
        const auto& tool = core::kTools[index];
        if (!core::canEnable(tool)) return;
        auto* mod = Mod::get();
        const std::string key(tool.id);
        mod->setSettingValue<bool>(key, !mod->getSettingValue<bool>(key));
        this->rebuild();
    }
};

}  // namespace

class $modify(GeoDanToolsPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto* playLayer = PlayLayer::get();
        if (!playLayer || !playLayer->m_level) return;

        // Own menu in the bottom-right corner, above the pause content, so the
        // button does not depend on another menu's layout.
        auto label = CCLabelBMFont::create("TOOLS", "bigFont.fnt");
        if (!label) return;
        label->setScale(0.32f);
        auto button = CCMenuItemSpriteExtra::create(
            label, this, menu_selector(GeoDanToolsPauseLayer::onTools));
        if (!button) return;
        button->setTag(kToolsButtonTag);
        auto menu = CCMenu::create();
        if (!menu) return;
        menu->addChild(button);
        const auto win = CCDirector::get()->getWinSize();
        menu->setPosition(ccp(win.width - 60.f, 30.f));
        menu->setZOrder(1000);
        this->addChild(menu);
    }

    void onTools(CCObject*) {
        if (auto* popup = ToolsPopup::create()) popup->show();
    }
};

// TOOLS on the level page, next to the level's play controls.
class $modify(GeoDanToolsLevelInfo, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        auto label = CCLabelBMFont::create("TOOLS", "bigFont.fnt");
        if (!label) return true;
        label->setScale(0.32f);
        auto button = CCMenuItemSpriteExtra::create(
            label, this, menu_selector(GeoDanToolsLevelInfo::onTools));
        if (!button) return true;
        button->setTag(kToolsButtonTag);
        auto menu = CCMenu::create();
        if (!menu) return true;
        menu->addChild(button);
        const auto win = CCDirector::get()->getWinSize();
        menu->setPosition(ccp(win.width - 60.f, 30.f));
        menu->setZOrder(1000);
        this->addChild(menu);
        return true;
    }

    void onTools(CCObject*) {
        if (auto* popup = ToolsPopup::create()) popup->show();
    }
};
