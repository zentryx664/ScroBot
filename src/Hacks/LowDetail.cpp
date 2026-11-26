#include "Core/Tick.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <type_traits>
#include <vector>
using namespace geode::prelude;

// Low Detail Mode 2
// Only objects inside a window around the screen are drawn. Objects the player has passed (left of the window)
// are hidden, and objects further ahead are held back and switched on again as they approach the window.
// Dying or restarting brings everything back, then the window is rebuilt from the start of the level.
// No game setting is changed and nothing is deleted, so physics, triggers and collisions are untouched.
static scro::Option& opt = scro::toggle("Visuals", "lowdetail2", "Low Detail Mode 2",
    "Draws only objects near the screen: passed objects are hidden and upcoming ones switch on as you reach them. Everything returns on death, restart or when turned off.",
    false, 20);

namespace {
constexpr float kMargin = 60.f;     // level units kept drawn outside each screen edge, so nothing pops in on screen

struct Entry { float x; GameObject* obj; };

std::vector<Entry> g_sorted;        // every object of the level, sorted by x
std::vector<unsigned char> g_hidden;// parallel to g_sorted: 1 = hidden by this feature
size_t g_lo = 0, g_hi = 0;          // indices outside [g_lo, g_hi) are (or are being) hidden
PlayLayer* g_layer = nullptr;
bool g_dirty = true;                // the list has to be (re)built

template <class C> void collect(C const& c, std::vector<Entry>& out) {
    if constexpr (std::is_pointer_v<std::remove_cvref_t<C>>) {
        if (c) for (auto o : CCArrayExt<GameObject*>(c)) { if (o) out.push_back({o->getPositionX(), o}); }
    } else {
        for (auto o : c) { if (o) out.push_back({o->getPositionX(), o}); }
    }
}

void restoreAll() {
    for (size_t i = 0; i < g_hidden.size(); ++i)
        if (g_hidden[i]) { g_sorted[i].obj->setVisible(true); g_hidden[i] = 0; }
    g_lo = 0;
    g_hi = g_sorted.size();
}

void forget() {                     // level is gone: the object pointers are no longer valid
    g_sorted.clear();
    g_hidden.clear();
    g_lo = g_hi = 0;
    g_layer = nullptr;
    g_dirty = true;
}

void rebuild(PlayLayer* pl) {
    g_sorted.clear();
    g_hidden.clear();
    collect(pl->m_objects, g_sorted);
    std::stable_sort(g_sorted.begin(), g_sorted.end(), [](Entry const& a, Entry const& b) { return a.x < b.x; });
    g_hidden.assign(g_sorted.size(), 0);
    g_lo = 0;
    g_hi = g_sorted.size();
    g_layer = pl;
    g_dirty = false;
}

void setRange(size_t from, size_t to, size_t lo, size_t hi) {
    for (size_t i = from; i < to && i < g_sorted.size(); ++i) {
        bool wantHidden = i < lo || i >= hi;
        auto* o = g_sorted[i].obj;
        if (wantHidden) {
            if (!g_hidden[i] && o->isVisible()) { o->setVisible(false); g_hidden[i] = 1; }
        } else if (g_hidden[i]) {
            o->setVisible(true);
            g_hidden[i] = 0;
        }
    }
}
}

static void lowDetailTick(PlayLayer* pl, float, bool) {
    if (pl != g_layer) forget();                                  // new level
    if (!opt.b) {
        if (std::any_of(g_hidden.begin(), g_hidden.end(), [](unsigned char h) { return h != 0; })) restoreAll();
        g_dirty = true;                                           // rebuild fresh next time it is turned on
        return;
    }
    auto layer = pl->m_objectLayer;
    if (!layer) return;
    if (g_dirty) rebuild(pl);
    if (g_sorted.empty()) return;

    // screen edges in level units (follows camera zoom and scale)
    auto win = CCDirector::get()->getWinSize();
    float left = layer->convertToNodeSpace(ccp(0.f, 0.f)).x - kMargin;
    float right = layer->convertToNodeSpace(ccp(win.width, 0.f)).x + kMargin;
    if (left > right) std::swap(left, right);

    auto first = std::lower_bound(g_sorted.begin(), g_sorted.end(), left,
        [](Entry const& e, float v) { return e.x < v; });
    auto last = std::upper_bound(g_sorted.begin(), g_sorted.end(), right,
        [](float v, Entry const& e) { return v < e.x; });
    size_t lo = static_cast<size_t>(first - g_sorted.begin());
    size_t hi = static_cast<size_t>(last - g_sorted.begin());
    if (lo == g_lo && hi == g_hi) return;

    // only the objects that crossed a window edge since the last frame are touched
    setRange(std::min(lo, g_lo), std::max(lo, g_lo), lo, hi);
    setRange(std::min(hi, g_hi), std::max(hi, g_hi), lo, hi);
    g_lo = lo;
    g_hi = hi;
}
static scro::tick::Reg regLowDetail(lowDetailTick);

class $modify(ScroLowDetail, PlayLayer) {
    void resetLevel() {
        restoreAll();                                             // objects ahead of a checkpoint are needed again
        PlayLayer::resetLevel();
        g_dirty = true;                                           // rebuild from the reset positions
    }
    void onQuit() {
        forget();
        PlayLayer::onQuit();
    }
};
