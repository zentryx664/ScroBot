#include "Hacks/FrameStep.hpp"
#include "Labels/Labels.hpp"
#include "Utils/Draw.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

static auto& freeze = scro::toggle("Global", "freeze", "Freeze Player", "Pause physics. Menus stay live.", false, 30);
static scro::Option& fstep = scro::toggle("Global", "framestep", "Frame Step", "Pause and show a step button.", false, 40);
static bool const globalSections = ((freeze.section = "General"), (fstep.section = "General"), true);
static bool g_pending = false;

bool scro::stepper::frozen() { return freeze.b || fstep.b; }
bool scro::stepper::consume() { bool p = g_pending; g_pending = false; return p; }
void scro::stepper::request() { g_pending = true; }

static bool _setup = (fstep.onChange = [] {
    if (auto pl = PlayLayer::get())
        if (auto b = scro::labels::menu(pl)->getChildByID("scrobot-step")) b->setVisible(fstep.b);
}, true);

class $modify(ScroStepLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        auto win = CCDirector::get()->getWinSize();
        auto b = CCMenuItemSpriteExtra::create(scro::draw::button(">", 28, 28), this, menu_selector(ScroStepLayer::onStep));
        b->setID("scrobot-step");
        b->setPosition(win.width - 24.f, 58.f);
        b->setVisible(fstep.b);
        scro::labels::menu(this)->addChild(b);
    }
    void onStep(CCObject*) { scro::stepper::request(); }
};
