// GeoDan Tools - Geode layer. Only draws the TOOLS popup and reads and writes
// settings; the tool list and its rules live in core/tool_registry.hpp.
#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/ui/Popup.hpp>

#include <string>

#include "core/tool_registry.hpp"

using namespace geode::prelude;
namespace core = geodan::core;

namespace {

constexpr float kWidth = 420.f;
constexpr float kHeight = 270.f;
constexpr float kCardWidth = 190.f;
constexpr float kCardHeight = 52.f;
constexpr float kGapX = 20.f;
constexpr float kGapY = 10.f;

// Saved value, or the default from mod.json when nothing is saved yet.
bool savedOrDefault(std::string_view id, bool) {
    return Mod::get()->getSettingValue<bool>(std::string(id));
}

class ToolsPopup : public geode::Popup {
    CCNode* m_cards = nullptr;

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
        this->setTitle("GeoDan Tools");
        this->rebuild();
        return true;
    }

    // Two columns of cards. Rebuilt after every change, so labels match settings.
    void rebuild() {
        if (m_cards) m_cards->removeFromParent();
        m_cards = CCNode::create();
        m_mainLayer->addChild(m_cards, 5);

        const auto size = m_mainLayer->getContentSize();
        const float left = (size.width - (kCardWidth * 2.f + kGapX)) / 2.f;
        const float top = size.height - 62.f;

        for (std::size_t i = 0; i < core::kToolCount; ++i) {
            const auto& tool = core::kTools[i];
            const float col = static_cast<float>(i % 2);
            const float row = static_cast<float>(i / 2);
            const float x = left + col * (kCardWidth + kGapX);
            const float y = top - row * (kCardHeight + kGapY);
            addCard(i, tool, x, y);
        }

        auto* summary = CCLabelBMFont::create(
            ("On: " + std::to_string(core::countOn(savedOrDefault))).c_str(), "chatFont.fnt");
        summary->setScale(0.5f);
        summary->setColor(ccc3(190, 190, 190));
        summary->setPosition(ccp(size.width / 2.f, 16.f));
        m_cards->addChild(summary);
    }

    void addCard(std::size_t index, const core::ToolDef& tool, float x, float y) {
        auto* card = CCNode::create();
        card->setPosition(ccp(x, y - kCardHeight));
        m_cards->addChild(card);

        const bool available = core::canEnable(tool);
        const bool on = core::isOn(tool, savedOrDefault);

        // Card body: the whole card is a button when the tool can be switched on.
        auto* title = CCLabelBMFont::create(std::string(tool.title).c_str(), "bigFont.fnt");
        title->setScale(0.3f);
        title->setAnchorPoint(ccp(0.f, 0.5f));
        title->setPosition(ccp(10.f, kCardHeight - 14.f));
        if (!available) title->setColor(ccc3(120, 120, 120));
        card->addChild(title);

        auto* blurb = CCLabelBMFont::create(std::string(tool.blurb).c_str(), "chatFont.fnt");
        blurb->setScale(0.42f);
        blurb->setAnchorPoint(ccp(0.f, 0.5f));
        blurb->setPosition(ccp(10.f, kCardHeight - 34.f));
        blurb->setColor(ccc3(170, 170, 170));
        card->addChild(blurb);

        const char* badge = !available ? "PLANNED" : (on ? "ON" : "OFF");
        auto* badgeLabel = CCLabelBMFont::create(badge, "bigFont.fnt");
        badgeLabel->setScale(0.26f);
        badgeLabel->setColor(!available ? ccc3(110, 110, 110)
                             : on ? ccc3(80, 220, 120) : ccc3(255, 200, 70));
        auto* button = CCMenuItemSpriteExtra::create(badgeLabel, this,
            menu_selector(ToolsPopup::onToggle));
        button->setTag(static_cast<int>(index));
        button->setEnabled(available);
        auto* menu = CCMenu::create();
        menu->addChild(button);
        menu->setPosition(ccp(kCardWidth - 30.f, kCardHeight / 2.f));
        card->addChild(menu);
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

        auto menu = CCMenu::create();
        auto label = CCLabelBMFont::create("TOOLS", "bigFont.fnt");
        if (!menu || !label) return;
        label->setScale(0.32f);
        auto button = CCMenuItemSpriteExtra::create(
            label, this, menu_selector(GeoDanToolsPauseLayer::onTools));
        if (!button) return;
        menu->addChild(button);
        // Bottom-right corner, clear of the pause menu's own buttons.
        const auto win = CCDirector::get()->getWinSize();
        menu->setPosition(ccp(win.width - 50.f, 30.f));
        this->addChild(menu);
    }

    void onTools(CCObject*) {
        if (auto* popup = ToolsPopup::create()) popup->show();
    }
};
