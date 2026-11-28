#include "Core/Tick.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <type_traits>
#include <unordered_map>
#include <vector>
using namespace geode::prelude;

// Low Detail Mode 2
// Plain blocks and decoration that have scrolled off the left edge are taken out of the scene graph
// (removed from the sprite batch), so the GPU no longer receives their quads at all. That is the real saving:
// a batch node submits every quad it holds each frame, even for hidden objects.
//
// What is and is not touched:
//  - Only Solid / Slope / Decoration objects that are not triggers. Triggers, hazards, portals, pads, orbs,
//    coins and every other gameplay object stay exactly as they are.
//  - Objects stay in the game's object list and collision sections, so physics, triggers and groups keep
//    working; only the drawing is gone.
//  - Death, restart, checkpoint respawn, turning the option off and leaving the level put every removed
//    object back exactly where it was (same parent, same layer order).
//  - If a trigger moves a removed object back on screen it is re-attached automatically.
//  - Platformer levels (the player can walk backwards) are left alone.
static scro::Option& opt = scro::toggle("Visuals", "lowdetail2", "Low Detail Mode 2",
    "Deletes blocks and decoration you have already passed (triggers and gameplay objects are never touched). They come back on death, restart or when turned off.",
    false, 20);

namespace {
constexpr float kMargin = 90.f;            // level units kept behind the left screen edge
constexpr size_t kDetachPerFrame = 700;    // spreads the first big sweep over a few frames
constexpr size_t kScanPerFrame = 256;      // round-robin check for removed objects that moved back

struct Entry { float x; GameObject* obj; };

struct Saved {                              // everything needed to put one object back
    Ref<GameObject> keep;                   // keeps the object alive while it is out of the scene
    Ref<CCNode> parent;
    int z = 0, tag = 0;
    Ref<CCNode> glow, glowParent;           // the glow lives in a different layer than the object
    int gz = 0, gtag = 0;
};

std::vector<Entry> g_sorted;                // plain blocks / decoration, sorted by x
std::vector<unsigned char> g_out;           // parallel: 1 = currently removed from the scene
std::unordered_map<size_t, Saved> g_saved;
size_t g_lo = 0;                            // indices below g_lo have been processed (removed)
size_t g_scan = 0;
PlayLayer* g_layer = nullptr;
bool g_dirty = true;

bool trackable(GameObject* o) {
    if (!o || o->m_isTrigger) return false;
    auto t = o->m_objectType;
    return t == GameObjectType::Solid || t == GameObjectType::Decoration || t == GameObjectType::Slope;
}

template <class C> void collect(C const& c, std::vector<Entry>& out) {
    if constexpr (std::is_pointer_v<std::remove_cvref_t<C>>) {
        if (c) for (auto o : CCArrayExt<GameObject*>(c)) { if (trackable(o)) out.push_back({o->getPositionX(), o}); }
    } else {
        for (auto o : c) { if (trackable(o)) out.push_back({o->getPositionX(), o}); }
    }
}

void detach(size_t i) {
    if (g_out[i]) return;
    auto* o = g_sorted[i].obj;
    auto* parent = o->getParent();
    if (!parent) return;                                   // not drawn anyway
    Saved s;
    s.keep = o;
    s.parent = parent;
    s.z = o->getZOrder();
    s.tag = o->getTag();
    if (auto* g = o->m_glowSprite) {
        if (auto* gp = g->getParent()) {
            s.glow = g;
            s.glowParent = gp;
            s.gz = g->getZOrder();
            s.gtag = g->getTag();
        }
    }
    o->removeFromParentAndCleanup(false);                  // false: keep its actions and schedules
    if (s.glow) static_cast<CCNode*>(s.glow)->removeFromParentAndCleanup(false);
    g_saved.emplace(i, std::move(s));
    g_out[i] = 1;
}

void attach(size_t i) {
    if (i >= g_out.size() || !g_out[i]) return;
    auto it = g_saved.find(i);
    if (it != g_saved.end()) {
        auto& s = it->second;
        auto* o = g_sorted[i].obj;
        if (!o->getParent() && s.parent) s.parent->addChild(o, s.z, s.tag);
        if (s.glow && !s.glow->getParent() && s.glowParent) s.glowParent->addChild(s.glow, s.gz, s.gtag);
        g_saved.erase(it);
    }
    g_out[i] = 0;
}

void restoreAll() {                                        // only call while the level is still alive
    for (size_t i = 0; i < g_out.size(); ++i) if (g_out[i]) attach(i);
    g_saved.clear();
    g_lo = 0;
    g_scan = 0;
}

void forget() {                                            // the level is gone: just drop everything
    g_saved.clear();
    g_sorted.clear();
    g_out.clear();
    g_lo = g_scan = 0;
    g_layer = nullptr;
    g_dirty = true;
}

void rebuild(PlayLayer* pl) {
    g_saved.clear();
    g_sorted.clear();
    collect(pl->m_objects, g_sorted);
    std::stable_sort(g_sorted.begin(), g_sorted.end(), [](Entry const& a, Entry const& b) { return a.x < b.x; });
    g_out.assign(g_sorted.size(), 0);
    g_lo = g_scan = 0;
    g_layer = pl;
    g_dirty = false;
}
}

static void lowDetailTick(PlayLayer* pl, float, bool) {
    if (pl != g_layer && !g_saved.empty()) forget();       // a different level: nothing to put back
    bool platformer = pl->m_level && pl->m_level->isPlatformer();
    if (!opt.b || platformer) {
        if (pl == g_layer && !g_saved.empty()) restoreAll();
        g_dirty = true;                                    // rebuild fresh next time it is turned on
        return;
    }
    auto layer = pl->m_objectLayer;
    if (!layer) return;
    if (pl != g_layer) g_dirty = true;
    if (g_dirty) rebuild(pl);
    if (g_sorted.empty()) return;

    // left screen edge in level units, correct for camera zoom and mirrored views
    auto win = CCDirector::get()->getWinSize();
    float a = layer->convertToNodeSpace(ccp(0.f, 0.f)).x;
    float b = layer->convertToNodeSpace(ccp(win.width, 0.f)).x;
    float left = std::min(a, b) - kMargin;

    auto first = std::lower_bound(g_sorted.begin(), g_sorted.end(), left,
        [](Entry const& e, float v) { return e.x < v; });
    size_t lo = static_cast<size_t>(first - g_sorted.begin());

    if (lo > g_lo) {                                       // camera moved right: remove what was passed
        size_t end = std::min(lo, g_lo + kDetachPerFrame);
        for (size_t i = g_lo; i < end; ++i) detach(i);
        g_lo = end;
    } else if (lo < g_lo) {                                // camera moved back: put objects back as they return
        for (size_t i = g_lo; i-- > lo;) attach(i);
        g_lo = lo;
    }

    // a trigger may have moved a removed object back into view: re-attach it
    if (g_lo > 0 && !g_saved.empty()) {
        size_t n = std::min(kScanPerFrame, g_lo);
        for (size_t k = 0; k < n; ++k) {
            size_t i = (g_scan + k) % g_lo;
            if (g_out[i] && g_sorted[i].obj->getPositionX() >= left) attach(i);
        }
        g_scan = (g_scan + n) % g_lo;
    }
}
static scro::tick::Reg regLowDetail(lowDetailTick);

class $modify(ScroLowDetail, PlayLayer) {
    void setupHasCompleted() {
        forget();                                          // a fresh level: drop anything from the last one
        PlayLayer::setupHasCompleted();
    }
    void resetLevel() {
        if (this == g_layer && !g_saved.empty()) restoreAll();   // death / restart / checkpoint: everything returns
        PlayLayer::resetLevel();
    }
    void onQuit() {
        if (this == g_layer && !g_saved.empty()) restoreAll();
        forget();
        PlayLayer::onQuit();
    }
};
