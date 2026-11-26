#include "Analysis/ClickAnalyzer.hpp"
#include "Core/Tick.hpp"
#include "Labels/Labels.hpp"
#include "UI/Style.hpp"
#include "Utils/Settings.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <fmt/format.h>
using namespace scro;

static auto& opt = toggle("Labels", "clickan", "Click Analyzer",
    "Shows the frame window of every click: which physics step it landed on, its position in the window, and its input delay.", false, 80);
static auto& resetOpt = action("Labels", "clickreset", "Reset Click Stats",
    "Clears the click statistics.", [] { scro::clicks::reset(); }, 90);

namespace {
int64_t nowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct Stats {
    // the frame window being played right now
    uint64_t frame = 0;
    int steps = 0, halves = 0, clicks = 0;
    bool curHalf = false;                    // the step being processed is a half step
    int64_t frameStart = 0;
    float frameMs = 0.f;
    // the previous window (its step count is the best guess for this one)
    int prevSteps = 0, prevClicks = 0;
    // arrival times of taps waiting to be applied
    std::deque<int64_t> arrivals;
    // last click
    bool any = false;
    uint64_t lastFrame = 0;
    int lastStep = 0, lastOf = 1;
    bool lastHalf = false;
    float lastInMs = 0.f, lastPos = 0.f, lastDelay = -1.f;
    // totals
    int total = 0, bins[3] = {0, 0, 0}, delayN = 0;
    float sumPos = 0.f, sumDelay = 0.f, worstDelay = 0.f;
    bool dirty = true;
};
Stats g;
Ref<CCLabelBMFont> g_label;
}

void scro::clicks::reset() { g = Stats(); }

void scro::clicks::frameBegin(float dt) {
    if (!opt.b) return;
    g.frame++;
    g.steps = g.halves = g.clicks = 0;
    g.frameStart = nowNs();
    g.frameMs = dt * 1000.f;
}
void scro::clicks::frameEnd() {
    if (!opt.b) return;
    g.prevSteps = g.steps;
    g.prevClicks = g.clicks;
    if (g.frame % 15 == 0) g.dirty = true;                  // keep the window size fresh
}
void scro::clicks::step(bool halfTick) {
    if (!opt.b) return;
    g.curHalf = halfTick;
    if (halfTick) g.halves++; else g.steps++;
}
void scro::clicks::arrive() {
    if (!opt.b) return;
    if (g.arrivals.size() < 16) g.arrivals.push_back(nowNs());
}
void scro::clicks::applied(bool down) {
    if (!opt.b) return;
    int64_t now = nowNs();
    float delay = -1.f;
    if (!g.arrivals.empty()) {                              // match this press/release with its tap
        delay = static_cast<float>(now - g.arrivals.front()) / 1e6f;
        g.arrivals.pop_front();
    }
    if (!down) return;                                      // statistics count presses only

    int of = std::max({g.prevSteps, g.steps, 1});           // steps in this window
    int slot = std::max(g.steps, 1);
    bool half = g.curHalf;
    float pos = std::clamp((static_cast<float>(slot - 1) + (half ? 0.5f : 0.f)) / static_cast<float>(of), 0.f, 0.999f);
    g.clicks++;
    g.any = true;
    g.lastFrame = g.frame;
    g.lastStep = slot; g.lastOf = of; g.lastHalf = half;
    g.lastInMs = static_cast<float>(now - g.frameStart) / 1e6f;
    g.lastPos = pos;
    g.lastDelay = delay;
    g.total++;
    g.sumPos += pos;
    g.bins[std::min(2, static_cast<int>(pos * 3.f))]++;
    if (delay >= 0.f) { g.delayN++; g.sumDelay += delay; g.worstDelay = std::max(g.worstDelay, delay); }
    g.dirty = true;
}

static void clickTick(PlayLayer* pl, float, bool) {
    if (!opt.b) {
        if (g_label && g_label->getParent()) { g_label->removeFromParent(); }
        g_label = nullptr;
        return;
    }
    auto hud = labels::hud(pl);
    if (!g_label || g_label->getParent() != hud) {
        auto win = CCDirector::get()->getWinSize();
        g_label = style::label("", (style::kMobile ? 10.f : 9.f), {255, 255, 255}, {0.f, 1.f}, {8.f, win.height - 58.f});
        g_label->setAlignment(kCCTextAlignmentLeft);
        hud->addChild(g_label);
        g.dirty = true;
    }
    if (!g.dirty) return;
    g.dirty = false;

    float stepMs = g.prevSteps > 0 ? g.frameMs / static_cast<float>(g.prevSteps) : 0.f;
    std::string t = fmt::format("Window #{}: {:.1f} ms, {} steps ({:.2f} ms each)\n",
        g.frame, g.frameMs, g.prevSteps, stepMs);
    if (g.any) {
        t += fmt::format("Last click: window #{}, step {}{}/{} ({:.0f}%), {:.1f} ms in\n",
            g.lastFrame, g.lastStep, g.lastHalf ? ".5" : "", g.lastOf, g.lastPos * 100.f, g.lastInMs);
        t += g.lastDelay >= 0.f ? fmt::format("Input delay: {:.1f} ms (avg {:.1f}, worst {:.1f})\n",
                  g.lastDelay, g.delayN ? g.sumDelay / static_cast<float>(g.delayN) : 0.f, g.worstDelay)
                                : std::string("Input delay: n/a on PC\n");
        t += fmt::format("Clicks {} | avg position {:.0f}% | early {} mid {} late {}",
            g.total, g.sumPos / static_cast<float>(g.total) * 100.f, g.bins[0], g.bins[1], g.bins[2]);
    } else {
        t += "No clicks yet";
    }
    g_label->setString(t.c_str());
}
static scro::tick::Reg regClick(clickTick);
