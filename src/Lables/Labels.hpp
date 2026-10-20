#pragma once
#include <Geode/Geode.hpp>
using namespace geode::prelude;

namespace scro::labels {
CCNode* hud(PlayLayer* pl);            // created on demand, safe to call from any hook
CCMenu* menu(PlayLayer* pl);           // button menu inside the HUD
CCLabelBMFont* info(PlayLayer* pl);    // centred banner label
void click();                          // call on every press (CPS)
}
