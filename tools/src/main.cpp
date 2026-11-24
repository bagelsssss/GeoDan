// GeoDan Tools - entry point: a "TOOLS" button on the pause menu opens the popup.
#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/ui/Popup.hpp>

#include "tools_contract.hpp"

using namespace geode::prelude;
namespace tools = geodan::tools;

namespace {

constexpr float kPopupWidth = 340.f;
constexpr float kPopupHeight = 300.f;
constexpr float kRowStart = 230.f;
constexpr float kRowGap = 30.f;

class ToolsPopup : public geode::Popup {
    CCNode* m_rows = nullptr;

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
        if (!Popup::init(kPopupWidth, kPopupHeight)) return false;
        this->setTitle("GeoDan Tools");
        this->rebuild();
        return true;
    }

    // Rebuilt after each change so every label matches the saved setting.
    void rebuild() {
        if (m_rows) m_rows->removeFromParent();
        m_rows = CCNode::create();
        m_mainLayer->addChild(m_rows, 5);

        const auto size = m_mainLayer->getContentSize();
        auto* mod = Mod::get();

        for (std::size_t i = 0; i < tools::kToggleCount; ++i) {
            const auto& t = tools::kToggles[i];
            const float y = size.height - kRowStart + kRowGap * static_cast<float>(i) + 40.f;

            auto* name = CCLabelBMFont::create(std::string(t.label).c_str(), "bigFont.fnt");
            name->setScale(0.32f);
            name->setAnchorPoint(ccp(0.f, 0.5f));
            name->setPosition(ccp(24.f, y));
            m_rows->addChild(name);

            const bool on = mod->getSettingValue<bool>(std::string(t.key));
            const char* text = !tools::canEnable(t) ? "PLANNED" : (on ? "ON" : "OFF");
            auto* label = CCLabelBMFont::create(text, "bigFont.fnt");
            label->setScale(0.32f);
            auto* button = CCMenuItemSpriteExtra::create(label, this,
                menu_selector(ToolsPopup::onToggle));
            button->setTag(static_cast<int>(i));
            button->setEnabled(tools::canEnable(t));
            if (!tools::canEnable(t)) label->setColor(ccc3(120, 120, 120));

            auto* menu = CCMenu::create();
            menu->addChild(button);
            menu->setPosition(ccp(kPopupWidth - 70.f, y));
            m_rows->addChild(menu);
        }
    }

    void onToggle(CCObject* sender) {
        const auto index = static_cast<std::size_t>(static_cast<CCNode*>(sender)->getTag());
        if (index >= tools::kToggleCount) return;
        const auto& t = tools::kToggles[index];
        if (!tools::canEnable(t)) return;
        auto* mod = Mod::get();
        const std::string key(t.key);
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
        if (!menu) return;
        auto label = CCLabelBMFont::create("TOOLS", "bigFont.fnt");
        if (!label) return;
        label->setScale(0.32f);
        auto button = CCMenuItemSpriteExtra::create(
            label, this, menu_selector(GeoDanToolsPauseLayer::onTools));
        if (!button) return;
        button->setPosition(ccp(0.f, 0.f));
        menu->addChild(button);
        this->addChild(menu);
    }

    void onTools(CCObject*) {
        if (auto* popup = ToolsPopup::create()) popup->show();
    }
};
