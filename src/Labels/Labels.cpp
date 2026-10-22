#include "Labels/Labels.hpp"
#include "Core/Tick.hpp"
#include "Macro/Bot.hpp"
#include "UI/Theme.hpp"
#include "Utils/Draw.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <ctime>
#include <deque>
using namespace scro;
using Clock = std::chrono::steady_clock;

static auto& oFps = toggle("Labels", "lfps", "FPS", "Frames per second.", true, 10);
static auto& oCps = toggle("Labels", "lcps", "CPS", "Clicks in the last second.", false, 20);
static auto& oAtt = toggle("Labels", "lattempt", "Attempt", "Current attempt number.", false, 30);
static auto& oTime = toggle("Labels", "ltime", "Run Time", "Time since the last respawn.", false, 40);
static auto& oClock = toggle("Labels", "lclock", "Clock", "Current time of day.", false, 50);
static auto& oFrame = toggle("Labels", "lframe", "Frame", "Current physics frame.", false, 60);
static auto& oOpac = step("Labels", "lopacity", "Label Opacity", "", 70.f, 10.f, 100.f, 5.f, "%", 70);

namespace {
Ref<CCNode> g_hud; Ref<CCMenu> g_menu;
Ref<CCLabelBMFont> g_stats, g_macro, g_info;
float g_fps = 60.f, g_run = 0.f, g_pulse = 0.f, g_acc = 0.f;
std::string g_lastStats, g_lastMacro;
std::deque<Clock::time_point> g_clicks;
}

CCNode* scro::labels::hud(PlayLayer* pl) {
    if (g_hud && g_hud->getParent() == pl->m_uiLayer) return g_hud;
    auto win = CCDirector::get()->getWinSize();
    auto hud = CCNode::create();
    hud->setID("scrobot-hud");
    g_stats = draw::label("", 0.7f, theme::TEXT, {0.f, 1.f}, {8.f, win.height - 8.f});
    g_macro = draw::label("", 0.7f, theme::TEXT, {0.f, 1.f}, {8.f, win.height - 24.f});
    g_info = draw::label("", 0.8f, theme::TEXT, {0.5f, 1.f}, {win.width / 2, win.height - 30.f});
    auto m = CCMenu::create();
    m->setPosition(0, 0);
    hud->addChild(g_stats); hud->addChild(g_macro); hud->addChild(g_info); hud->addChild(m);
    pl->m_uiLayer->addChild(hud, 100);
    g_hud = hud; g_menu = m;
    g_lastStats.clear(); g_lastMacro.clear();
    return hud;
}
CCMenu* scro::labels::menu(PlayLayer* pl) { hud(pl); return g_menu; }
CCLabelBMFont* scro::labels::info(PlayLayer* pl) { hud(pl); return g_info; }
void scro::labels::click() { g_clicks.push_back(Clock::now()); }

static void tickLabels(PlayLayer* pl, float dt, bool frozen) {
    labels::hud(pl);
    if (dt > 0.f) g_fps = g_fps * 0.9f + (1.f / dt) * 0.1f;
    if (!frozen) g_run += dt;
    g_pulse += dt; g_acc += dt;
    if (g_acc < 0.1f) return;                      // text is rebuilt 10x/s at most
    g_acc = 0.f;

    auto& bot = Bot::get();
    std::string m;
    if (bot.state == BotState::Recording) m = fmt::format("REC  {} inputs", bot.actions.size());
    else if (bot.state == BotState::Playing) m = fmt::format("PLAY  frame {}", pl->m_gameState.m_currentProgress);
    if (m != g_lastMacro) {
        g_lastMacro = m;
        g_macro->setString(m.c_str());
        g_macro->setColor(bot.state == BotState::Recording ? ccColor3B{255, 90, 90} : ccColor3B{110, 255, 130});
    }
    if (bot.state == BotState::Recording) g_macro->setOpacity(static_cast<GLubyte>(170 + 80 * std::sin(g_pulse * 5.f)));

    if (!(oFps.b || oCps.b || oAtt.b || oTime.b || oClock.b || oFrame.b)) {
        if (!g_lastStats.empty()) { g_lastStats.clear(); g_stats->setString(""); }
        return;
    }
    auto now = Clock::now();
    while (!g_clicks.empty() && now - g_clicks.front() > std::chrono::seconds(1)) g_clicks.pop_front();
    std::string s;
    auto add = [&](std::string const& t) { if (!s.empty()) s += "    "; s += t; };
    if (oFps.b) add(fmt::format("FPS {}", static_cast<int>(g_fps + 0.5f)));
    if (oCps.b) add(fmt::format("CPS {}", g_clicks.size()));
    if (oAtt.b) add(fmt::format("Attempt {}", pl->m_attempts));
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
        g_hud = nullptr; g_menu = nullptr; g_stats = nullptr; g_macro = nullptr; g_info = nullptr;
        g_clicks.clear();
        PlayLayer::onQuit();
    }
};
