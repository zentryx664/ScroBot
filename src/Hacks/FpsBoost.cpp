#include "Hacks/FpsBoost.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <algorithm>
#include <chrono>
using namespace geode::prelude;
using Clock = std::chrono::steady_clock;

// ---- options (own category) --------------------------------------------------------------
static scro::Option& boost = scro::toggle("FPS Boost", "fpsboost", "FPS Boost",
    "Lifts the frame rate cap so the game runs at your target FPS. Turn VSync off in GD's video settings for best results.", false, 10);
static scro::Option& target = scro::step("FPS Boost", "fpstarget", "Target FPS",
    "How high the game may run. Higher is smoother but uses more power.", 5.f, 0.f, 9.f, 1.f, "", 20);
static scro::Option& precision = scro::toggle("FPS Boost", "clickprec", "Click Precision",
    "Applies every click at the exact moment you pressed it, even on laggy frames, so timing stays perfect.", true, 30);
static scro::Option& diagnostics = scro::toggle("FPS Boost", "fpsdiag", "Frame-Time Diagnostics",
    "Show smoothed frame time and recent frame-time peak in the in-level HUD.", false, 40);
static scro::Option& lowEndCap = scro::toggle("FPS Boost", "lowendcap", "Low-End Safe Cap (120 FPS)",
    "When enabled, Scro caps its requested target at 120 FPS. This is a ceiling, not a guarantee that the device can render 120 FPS.", true, 50);

static float const kPresets[] = {60, 90, 120, 144, 165, 240, 360, 480, 720, 1000};
static bool _setup = ([] {
    target.names = {"60", "90", "120", "144", "165", "240", "360", "480", "720", "1000"};
    for (auto* o : {&boost, &target}) o->section = "Frame Rate";
    precision.section = "Input";
    diagnostics.section = "Diagnostics";
    boost.onChange = target.onChange = lowEndCap.onChange = [] { scro::fps::apply(); };
}(), true);

// ---- FPS ---------------------------------------------------------------------------------
static bool g_saved = false, g_origVar = false;
static float g_origFps = 60.f;

void scro::fps::apply() {
    auto gm = GameManager::sharedState();
    if (!gm) return;
    if (!g_saved) { g_saved = true; g_origFps = gm->m_customFPSTarget; g_origVar = gm->getGameVariable("0116"); }
    int preset = std::clamp(static_cast<int>(target.f), 0, 9);
    // The low-end profile is a safe ceiling. It cannot create extra performance;
    // it only prevents Scro from requesting an unnecessarily high target.
    if (lowEndCap.b) preset = std::min(preset, 2); // 60 / 90 / 120 FPS
    float fps = boost.b ? kPresets[preset] : g_origFps;
    gm->setGameVariable("0116", boost.b ? true : g_origVar);
    gm->m_customFPSTarget = fps;
    CCApplication::sharedApplication()->setAnimationInterval(1.0 / fps);
}

class $modify(ScroFpsMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (boost.b) scro::fps::apply();       // restore the saved setting on startup
        return true;
    }
};

// ---- Click Precision ---------------------------------------------------------------------
// Presses are timestamped when they arrive and released to the game at the exact point inside
// the frame (see TickDriver), instead of all landing at the start of a laggy frame.
namespace {
struct Raw { int b; bool d, p; Clock::time_point t; };
std::vector<Raw> g_held;
Clock::time_point g_last = Clock::now();
}
bool scro::fps::precise() { return boost.b && precision.b; }
bool scro::fps::pending() { return !g_held.empty(); }
void scro::fps::hold(int b, bool d, bool p) { g_held.push_back({b, d, p, Clock::now()}); }

std::vector<scro::fps::Held> scro::fps::drain() {
    auto now = Clock::now();
    float span = std::chrono::duration<float>(now - g_last).count();
    std::vector<Held> out;
    out.reserve(g_held.size());
    for (auto& r : g_held) {
        float f = 0.f;                                          // stale (e.g. after a pause) -> start of frame
        if (span > 0.f && span < 0.25f) f = std::clamp(std::chrono::duration<float>(r.t - g_last).count() / span, 0.f, 1.f);
        out.push_back({r.b, r.d, r.p, f});
    }
    g_held.clear();
    g_last = now;
    return out;
}

class $modify(ScroFpsQueue, GJBaseGameLayer) {
    void queueButton(int button, bool down, bool isPlayer1, double timestamp) {
        if (!scro::fps::g_bypass && scro::fps::precise() && PlayLayer::get())
            return scro::fps::hold(button, down, isPlayer1);
        GJBaseGameLayer::queueButton(button, down, isPlayer1, timestamp);
    }
};
