#include "Labels/Labels.hpp"
#include "Utils/Draw.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

static scro::Option& qr = scro::toggle("Utility", "quickrestart", "Quick Restart", "Adds a restart button in levels.", false, 10);
static bool _setup = (qr.onChange = [] {
    if (auto pl = PlayLayer::get())
        if (auto b = scro::labels::menu(pl)->getChildByID("scrobot-restart")) b->setVisible(qr.b);
}, true);

class $modify(ScroRestartLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        auto win = CCDirector::get()->getWinSize();
        auto b = CCMenuItemSpriteExtra::create(scro::draw::button("R", 28, 28), this, menu_selector(ScroRestartLayer::onQuickRestart));
        b->setID("scrobot-restart");
        b->setPosition(win.width - 24.f, 24.f);
        b->setVisible(qr.b);
        scro::labels::menu(this)->addChild(b);
    }
    void onQuickRestart(CCObject*) { this->resetLevel(); }
};
