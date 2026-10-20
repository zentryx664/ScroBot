#include "UI/Menu.hpp"
#include "UI/Theme.hpp"
#include "Utils/Draw.hpp"
#include <Geode/modify/PauseLayer.hpp>
using namespace geode::prelude;

class $modify(ScroPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto ac = scro::theme::accent();
        auto n = scro::draw::button("Scro", 64, 24);
        if (auto star = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png")) {
            star->setScale(0.3f); star->setColor(ac); star->setPosition(12, 13); n->addChild(star);
        }
        auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ScroPauseLayer::onScro));
        auto menu = CCMenu::create();
        menu->addChild(b);
        auto win = CCDirector::get()->getWinSize();
        menu->setPosition(46.f, win.height - 30.f);
        this->addChild(menu);
    }
    void onScro(CCObject*) { scro::ui::open(); }
};
