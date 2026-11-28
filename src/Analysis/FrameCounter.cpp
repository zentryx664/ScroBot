#include "Analysis/FrameCounter.hpp"
#include "Core/Tick.hpp"
#include "Labels/Labels.hpp"
#include "UI/Style.hpp"
#include "Utils/Settings.hpp"
#include <algorithm>
#include <fmt/format.h>
using namespace scro;

static auto& opt = toggle("Labels", "clickan", "Frame Counter",
    "Reads the frame window of every click (like a macro that also measures its timings). With a macro playing it counts every click of the run; off, macros just play normally.", false, 80);
static auto& soundOpt = toggle("Labels", "fcsound", "Counter Sound",
    "Plays a sound on every click. Fewer frames = lower pitch, more frames = higher pitch.", true, 81);
static auto& squareOpt = toggle("Labels", "fcsquare", "Counter Squares",
    "Marks every click with a square coloured by its frame window.", true, 82);
static auto& resetOpt = action("Labels", "clickreset", "Reset Frame Counter",
    "Clears the counters.", [] { scro::fc::reset(); }, 90);

namespace {
constexpr int kBins = 7;
char const* const kNames[kBins] = {"9-12:", "7-8:", "5-6:", "4:", "3:", "2:", "0-1:"};
ccColor3B const kColors[kBins] = {
    {140, 120, 255}, {110, 185, 255}, {120, 240, 130}, {255, 255, 255},
    {255, 240, 100}, {255, 170, 80},  {255, 70, 100},
};

int binOf(int w) {
    if (w >= 9) return 0;
    if (w >= 7) return 1;
    if (w >= 5) return 2;
    if (w == 4) return 3;
    if (w == 3) return 4;
    if (w == 2) return 5;
    return 6;
}

struct Counter {
    int counts[kBins] = {};
    int total = 0;
    bool dirty = true;
};
Counter g;

struct Hud {
    Ref<CCNode> root;
    CCLabelBMFont* name[kBins] = {};
    CCLabelBMFont* value[kBins] = {};
    CCLabelBMFont* total = nullptr;
} g_hud;

void spawnSquare(PlayerObject* p, ccColor3B c) {
    auto* parent = p ? p->getParent() : nullptr;
    if (!parent) return;
    auto n = CCDrawNode::create();
    float const s = 15.f;
    CCPoint v[4] = {{-s, -s}, {s, -s}, {s, s}, {-s, s}};
    ccColor4F col = ccc4f(c.r / 255.f, c.g / 255.f, c.b / 255.f, 1.f);
    n->drawPolygon(v, 4, ccc4f(col.r, col.g, col.b, 0.14f), 2.f, col);
    n->setPosition(p->getPosition());
    n->setScale(1.6f);
    parent->addChild(n, p->getZOrder() + 5);
    n->runAction(CCEaseExponentialOut::create(CCScaleTo::create(0.14f, 1.f)));
    n->runAction(CCSequence::create(CCDelayTime::create(1.8f), CCRemoveSelf::create(), nullptr));
}

void playClick(int window) {
    float t = static_cast<float>(std::clamp(window, 1, 12) - 1) / 11.f;      // 0 = tightest, 1 = most room
    float speed = 0.55f + 1.2f * t;                                          // low pitch .. high pitch
    if (auto* fmod = FMODAudioEngine::sharedEngine()) fmod->playEffect("playSound_01.ogg", speed, 0.f, 0.5f);
}

void build(CCNode* hud) {
    auto win = CCDirector::get()->getWinSize();
    float const scale = style::kMobile ? 0.34f : 0.30f;
    float const row = style::kMobile ? 14.f : 13.f;
    g_hud.root = CCNode::create();
    g_hud.root->setPosition(6.f, win.height - 48.f);
    for (int i = 0; i < kBins; ++i) {
        float y = -static_cast<float>(i) * row;
        auto name = CCLabelBMFont::create(kNames[i], "bigFont.fnt");
        name->setScale(scale);
        name->setColor(kColors[i]);
        name->setAnchorPoint({0.f, 0.5f});
        name->setPosition(0.f, y);
        g_hud.root->addChild(name);
        g_hud.name[i] = name;
        auto value = CCLabelBMFont::create("0", "bigFont.fnt");
        value->setScale(scale);
        value->setColor(kColors[i]);
        value->setAnchorPoint({1.f, 0.5f});
        value->setPosition(54.f, y);
        g_hud.root->addChild(value);
        g_hud.value[i] = value;
    }
    g_hud.total = CCLabelBMFont::create("Clicks: 0", "bigFont.fnt");
    g_hud.total->setScale(scale * 0.8f);
    g_hud.total->setColor({170, 170, 170});
    g_hud.total->setAnchorPoint({0.f, 0.5f});
    g_hud.total->setPosition(0.f, -static_cast<float>(kBins) * row - 2.f);
    g_hud.root->addChild(g_hud.total);
    hud->addChild(g_hud.root);
    g.dirty = true;
}
}

bool scro::fc::enabled() { return opt.b; }
void scro::fc::frameBegin(float) {}
void scro::fc::frameEnd() {}
void scro::fc::step(bool) {}
void scro::fc::arrive() {}

void scro::fc::reset() { g = Counter(); }

void scro::fc::pressed(int window, PlayerObject* player) {
    if (!opt.b) return;
    int w = std::clamp(window, 0, 12);
    int b = binOf(w);
    g.counts[b]++;
    g.total++;
    g.dirty = true;
    if (squareOpt.b) spawnSquare(player, kColors[b]);
    if (soundOpt.b) playClick(w);
}

static void counterTick(PlayLayer* pl, float, bool) {
    if (!opt.b) {
        if (g_hud.root) { g_hud.root->removeFromParent(); g_hud.root = nullptr; }
        return;
    }
    auto hud = labels::hud(pl);
    if (!g_hud.root || g_hud.root->getParent() != hud) build(hud);
    if (!g.dirty) return;
    g.dirty = false;
    for (int i = 0; i < kBins; ++i) g_hud.value[i]->setString(fmt::format("{}", g.counts[i]).c_str());
    g_hud.total->setString(fmt::format("Clicks: {}", g.total).c_str());
}
static scro::tick::Reg regCounter(counterTick);
