#include "Labels/Labels.hpp"
#include "Core/Tick.hpp"
#include "UI/Style.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <deque>
using namespace scro;
using Clock = std::chrono::steady_clock;

static auto& oFps = toggle("Labels", "lfps", "FPS", "Frames per second.", true, 10);
static auto& oCps = toggle("Labels", "lcps", "CPS", "Clicks in the last second.", false, 20);
static auto& oTime = toggle("Labels", "ltime", "Run Time", "Time since the last respawn.", false, 40);
static auto& oClock = toggle("Labels", "lclock", "Clock", "Current time of day.", false, 50);
static auto& oFrame = toggle("Labels", "lframe", "Frame", "Current physics frame.", false, 60);
static auto& oOpac = step("Labels", "lopacity", "Label Opacity", "", 70.f, 10.f, 100.f, 5.f, "%", 70);

namespace {
Ref<CCNode> g_hud;
Ref<CCLabelBMFont> g_stats, g_info, g_perf;
float g_fps = 60.f, g_run = 0.f, g_acc = 0.f;
float g_frameAvgMs = 1000.f / 60.f, g_framePeakMs = 0.f;
std::string g_lastStats, g_lastPerf;
std::deque<Clock::time_point> g_clicks;
}

namespace {
CCLabelBMFont* hudLabel(float line, CCPoint anchor, CCPoint pos) {
    return style::label("", line, {255, 255, 255}, anchor, pos);
}
}

CCNode* scro::labels::hud(PlayLayer* pl) {
    if (g_hud && g_hud->getParent() == pl->m_uiLayer) return g_hud;
    auto win = CCDirector::get()->getWinSize();
    float const line = style::kMobile ? 11.f : 10.f;
    auto hud = CCNode::create();
    hud->setID("scrobot-hud");
    g_stats = hudLabel(line, {0.f, 1.f}, {8.f, win.height - 8.f});
    g_info = hudLabel(line + 2.f, {0.5f, 1.f}, {win.width / 2, win.height - 30.f});
    g_perf = hudLabel(line - 2.f, {0.f, 1.f}, {8.f, win.height - 8.f - line * 1.6f});
    hud->addChild(g_stats);
    hud->addChild(g_info);
    hud->addChild(g_perf);
    pl->m_uiLayer->addChild(hud, 100);
    g_hud = hud;
    g_lastStats.clear();
    g_lastPerf.clear();
    return hud;
}
CCLabelBMFont* scro::labels::info(PlayLayer* pl) { hud(pl); return g_info; }
void scro::labels::click() { g_clicks.push_back(Clock::now()); }

static void tickLabels(PlayLayer* pl, float dt, bool frozen) {
    labels::hud(pl);
    static auto& perfDiag = find("fpsdiag");
    if (dt > 0.f) {
        g_fps = g_fps * 0.9f + (1.f / dt) * 0.1f;
        if (perfDiag.b) {
            float frameMs = dt * 1000.f;
            g_frameAvgMs = g_frameAvgMs * 0.9f + frameMs * 0.1f;
            g_framePeakMs = std::max(g_framePeakMs, frameMs);
        }
    }
    if (!frozen) g_run += dt;
    g_acc += dt;
    if (g_acc < 0.1f) return;                      // text is rebuilt 10x/s at most
    g_acc = 0.f;

    if (perfDiag.b) {
        std::string perf = fmt::format("FRAME {:.2f} ms   PEAK {:.2f} ms", g_frameAvgMs, g_framePeakMs);
        if (perf != g_lastPerf) { g_lastPerf = perf; g_perf->setString(perf.c_str()); }
        g_perf->setColor(g_framePeakMs > 1000.f / 30.f ? ccColor3B{255, 150, 100} : ccColor3B{255, 255, 255});
    } else if (!g_lastPerf.empty()) {
        g_lastPerf.clear();
        g_perf->setString("");
    }
    g_framePeakMs = 0.f;

    if (!(oFps.b || oCps.b || oTime.b || oClock.b || oFrame.b)) {
        if (!g_lastStats.empty()) { g_lastStats.clear(); g_stats->setString(""); }
        return;
    }
    auto now = Clock::now();
    while (!g_clicks.empty() && now - g_clicks.front() > std::chrono::seconds(1)) g_clicks.pop_front();
    std::string s;
    auto add = [&](std::string const& t) { if (!s.empty()) s += "    "; s += t; };
    if (oFps.b) add(fmt::format("FPS {}", static_cast<int>(g_fps + 0.5f)));
    if (oCps.b) add(fmt::format("CPS {}", g_clicks.size()));
    if (oTime.b) add(fmt::format("Time {:.1f}", g_run));
    if (oFrame.b) add(fmt::format("Frame {}", pl->m_gameState.m_currentProgress));
    if (oClock.b) {
        char buf[8]; std::time_t t = std::time(nullptr);
        std::strftime(buf, sizeof(buf), "%H:%M", std::localtime(&t));
        add(buf);
    }
    if (s != g_lastStats) { g_lastStats = s; g_stats->setString(s.c_str()); }
    g_stats->setOpacity(static_cast<GLubyte>(oOpac.f * 2.55f));
}
static tick::Reg regLabels(tickLabels);

class $modify(ScroLabelsLayer, PlayLayer) {
    void resetLevel() { PlayLayer::resetLevel(); g_run = 0.f; }
    void onQuit() {
        g_hud = nullptr; g_stats = nullptr; g_info = nullptr; g_perf = nullptr;
        g_clicks.clear();
        g_lastStats.clear(); g_lastPerf.clear();
        g_frameAvgMs = 1000.f / 60.f; g_framePeakMs = 0.f;
        PlayLayer::onQuit();
    }
};
