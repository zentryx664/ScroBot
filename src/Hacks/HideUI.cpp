#include <cstring>
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

static scro::Option& hide = scro::toggle("Visuals", "hideui", "Hide UI", "Hide the in-level interface.", false, 50);

static void applyHide(PlayLayer* pl) {
    if (!pl || !pl->m_uiLayer) return;
    if (auto ch = pl->m_uiLayer->getChildren())
        for (auto c : CCArrayExt<CCNode*>(ch))
            if (c->getID().rfind("scrobot", 0) != 0) c->setVisible(!hide.b);
}
static bool _setup = (hide.onChange = [] { applyHide(PlayLayer::get()); }, true);

class $modify(ScroHideUI, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        if (hide.b) applyHide(this);
    }
};
