#include "Core/Tick.hpp"
#include "Utils/Settings.hpp"
using namespace geode::prelude;

static scro::Option& hb = scro::toggle("Visuals", "hitboxes", "Hitboxes", "Draw collision outlines.", false, 10);
static bool _setup = (hb.onChange = [] {
    if (hb.b) return;
    if (auto pl = PlayLayer::get()) {
        pl->m_isDebugDrawEnabled = false;
        if (pl->m_debugDrawNode) pl->m_debugDrawNode->clear();
    }
}, true);
static scro::tick::Reg reg(+[](PlayLayer* pl, float, bool) {
    if (!hb.b) return;
    pl->m_isDebugDrawEnabled = true;
    pl->updateDebugDraw();
});
