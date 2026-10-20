#pragma once
#include <Geode/Geode.hpp>
using namespace geode::prelude;

namespace scro::draw {
CCLayerColor* rect(float w, float h, ccColor4B c);
CCLabelBMFont* label(std::string const& t, float scale, ccColor3B col, CCPoint anchor, CCPoint pos);
void fadeTree(CCNode* n, float delay);
CCNode* button(char const* text, float w, float h);                       // flat grey button
void triangle(CCNode* parent, bool down, float size, CCPoint pos, ccColor3B c);
CCNode* panel(float w, float h, float titleH);                            // dark body + accent title bar
}
