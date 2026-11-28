#include "Core/Tick.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/CCTextureCache.hpp>
using namespace geode::prelude;

// Smooth: a clean, flat look at full rendering quality (nothing is scaled down).
//  - textures are drawn with smooth (linear) filtering
//  - the level's background, middle layer and ground textures are hidden, leaving plain colour
//  - glow and particles are switched off (see NoGlow / NoParticles, they check this option too)
static scro::Option& smooth = scro::toggle("Visuals", "smooth", "Smooth",
    "Plain, smooth graphics at full quality: smooth texture filtering, no background / ground textures, no glow, no particles.",
    false, 25);

namespace {
void filterTree(CCNode* n, int depth) {
    if (!n || depth > 12) return;
    if (auto s = typeinfo_cast<CCSprite*>(n)) { if (auto t = s->getTexture()) t->setAntiAliasTexParameters(); }
    else if (auto b = typeinfo_cast<CCSpriteBatchNode*>(n)) { if (auto t = b->getTexture()) t->setAntiAliasTexParameters(); }
    if (auto ch = n->getChildren())
        for (auto c : CCArrayExt<CCNode*>(ch)) filterTree(c, depth + 1);
}
void filterScene() {
    if (auto scene = CCDirector::get()->getRunningScene()) filterTree(scene, 0);
}

bool g_hidBg = false, g_hidMg = false, g_hidGround = false;

void setHidden(CCNode* n, bool want, bool& flag) {
    if (!n) return;
    if (want && n->isVisible()) { n->setVisible(false); flag = true; }
    else if (!want && flag) { n->setVisible(true); flag = false; }   // only undo what we hid
}
bool const wired = (smooth.onChange = [] { if (smooth.b) filterScene(); }, true);
}

// textures loaded after the option is on get smooth filtering as they are created
class $modify(ScroSmoothTextures, CCTextureCache) {
    CCTexture2D* addImage(char const* path, bool fromCache) {
        auto tex = CCTextureCache::addImage(path, fromCache);
        if (tex && smooth.b) tex->setAntiAliasTexParameters();
        return tex;
    }
};

static scro::tick::Reg regSmooth(+[](PlayLayer* pl, float, bool) {
    static PlayLayer* filtered = nullptr;
    if (smooth.b && pl != filtered) { filtered = pl; filterScene(); }
    if (!smooth.b && filtered) filtered = nullptr;
    bool on = smooth.b;
    setHidden(pl->m_background, on, g_hidBg);
    setHidden(pl->m_middleground, on, g_hidMg);
    setHidden(pl->m_groundLayer, on, g_hidGround);
});
