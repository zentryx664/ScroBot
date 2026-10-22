#include "UI/Menu.hpp"
#include "UI/Theme.hpp"
#include "Utils/Draw.hpp"
#include <Geode/modify/PauseLayer.hpp>
using namespace geode::prelude;

class $modify(ScroPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto n = scro::draw::button("Scro", 54, 24);
        auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ScroPauseLayer::onScro));
        b->m_scaleMultiplier = 1.f;
        auto menu = CCMenu::create();
        menu->addChild(b);
        auto win = CCDirector::get()->getWinSize();
        menu->setPosition(46.f, win.height - 30.f);
        this->addChild(menu);
    }
    void onScro(CCObject*) { scro::ui::open(); }
};
