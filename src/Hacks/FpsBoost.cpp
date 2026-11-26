#include "Hacks/FpsBoost.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>
#include "Analysis/ClickAnalyzer.hpp"
#include "Core/Tick.hpp"

using namespace geode::prelude;
using Clock = std::chrono::steady_clock;

namespace {
struct FpsOptions {
    scro::Option& boost;
    scro::Option& target;
    scro::Option& boostFrames;
#ifdef GEODE_IS_MOBILE
    scro::Option& zeroDelay;
#endif
#ifdef GEODE_IS_ANDROID
    scro::Option& precision;
#endif
    scro::Option& diagnostics;

    FpsOptions()
      : boost(scro::toggle("FPS Boost", "fpsboost", "FPS Boost",
          "Forces the game to run at 120 FPS or higher (never below 120). Turn VSync off in the game's video settings for best results.", false, 10)),
        target(scro::step("FPS Boost", "fpstarget120", "Target FPS",
          "Frame rate to force. The minimum is 120.", 0.f, 0.f, 7.f, 1.f, "", 20)),
        boostFrames(scro::toggle("FPS Boost", "boostframes", "Boost Frames",
          "Click between frames: your taps register at their exact moment between physics steps, which makes cube jumps and wave turns easier. Also lightens effects. Completions are sent as test runs while it is on.", false, 25)),
#ifdef GEODE_IS_MOBILE
        zeroDelay(scro::toggle("FPS Boost", "zdcbf", "Zero-Delay CBF",
          "Mobile only. Uses the game's native click-between-steps behavior.", false, 28)),
#endif
#ifdef GEODE_IS_ANDROID
        precision(scro::toggle("FPS Boost", "clickprec", "Click Precision",
          "Applies every click at the exact moment you pressed it, even on laggy frames. Needs FPS Boost on.", false, 30)),
#endif
        diagnostics(scro::toggle("FPS Boost", "fpsdiag", "Frame-Time Diagnostics",
          "Show smoothed frame time and recent frame-time peak in the in-level HUD.", false, 40)) {
        target.names = {"120", "144", "165", "240", "360", "480", "720", "1000"};
        for (auto* option : {&boost, &target, &boostFrames}) option->section = "Frame Rate";
#ifdef GEODE_IS_MOBILE
        zeroDelay.section = "Input";
#endif
#ifdef GEODE_IS_ANDROID
        precision.section = "Input";
#endif
        diagnostics.section = "Diagnostics";

        // Safe Mode is no longer locked by Boost Frames. Completions are still sent as test runs while it is
        // active (see SafeMode.cpp), so nothing is ever submitted.
        boost.onChange = target.onChange = [] { scro::fps::apply(); };
    }
};

FpsOptions& options() {
    // Do not register settings from an ELF global constructor. On Android, Geode's
    // Mod/settings services are not ready until the Loaded callback.
    static FpsOptions value;
    return value;
}

constexpr float kPresets[] = {120.f, 144.f, 165.f, 240.f, 360.f, 480.f, 720.f, 1000.f};

struct RawClick {
    int button;
    bool down;
    bool player1;
    Clock::time_point time;
};

std::vector<RawClick>& heldQueue() {
    static std::vector<RawClick> value;
    return value;
}

Clock::time_point& lastClickTime() {
    static Clock::time_point value = Clock::now();
    return value;
}
} // namespace

void scro::fps::initOptions() {
    // Called from $on_mod(Loaded), before scro::load().
    (void)options();

    // Register tick callbacks only after the runtime and callback registry are ready.
    static bool registered = [] {
        static scro::tick::Reg regCbs(+[](PlayLayer* layer, float, bool) {
            if (!layer) return;
            auto& o = options();
            static bool saved = false;
            static bool originalBetweenSteps = false;
            static bool originalOnSteps = false;

            bool active = false;
#ifdef GEODE_IS_MOBILE
            active = o.zeroDelay.b;
#endif
            active = active || (o.boost.b && o.boostFrames.b);

            if (active) {
                if (!saved) {
                    saved = true;
                    originalBetweenSteps = layer->m_clickBetweenSteps;
                    originalOnSteps = layer->m_clickOnSteps;
                }
                layer->m_clickBetweenSteps = true;
                layer->m_clickOnSteps = false;
            } else if (saved) {
                saved = false;
                layer->m_clickBetweenSteps = originalBetweenSteps;
                layer->m_clickOnSteps = originalOnSteps;
            }
        });

        static scro::tick::Reg regFps(+[](PlayLayer*, float dt, bool) {
            static float elapsed = 0.f;
            elapsed += std::max(0.f, dt);
            if (elapsed < 1.f) return;
            elapsed = 0.f;

            auto& o = options();
            if (!o.boost.b) return;
            auto* gm = GameManager::sharedState();
            if (gm && std::fabs(gm->m_customFPSTarget -
                    kPresets[std::clamp(static_cast<int>(std::lround(o.target.f)), 0, 7)]) > 0.5f) {
                scro::fps::apply();
            }
        });

        (void)regCbs;
        (void)regFps;
        return true;
    }();
    (void)registered;
}

bool scro::fps::boostFrames() {
    auto& o = options();
    return o.boost.b && o.boostFrames.b;
}

[[maybe_unused]] static bool cbfActive() {
    auto& o = options();
#ifdef GEODE_IS_MOBILE
    if (o.zeroDelay.b) return true;
#endif
    return scro::fps::boostFrames();
}

// ---- FPS control -------------------------------------------------------------------------
static bool g_savedOriginal = false;
static bool g_originalUnlock = false;
static float g_originalFps = 60.f;

void scro::fps::apply() {
    auto& o = options();
    auto* gm = GameManager::sharedState();
    if (!gm) return;

    if (!g_savedOriginal) {
        g_originalFps = std::max(1.f, gm->m_customFPSTarget);
        g_originalUnlock = gm->getGameVariable("0116");
        g_savedOriginal = true;
    }

    const int preset = std::clamp(static_cast<int>(std::lround(o.target.f)), 0, 7);
    const float requestedFps = o.boost.b ? kPresets[preset] : g_originalFps;
    const bool unlock = o.boost.b ? true : g_originalUnlock;

    gm->setGameVariable("0116", unlock);
    gm->m_customFPSTarget = requestedFps;

    if (auto* app = CCApplication::sharedApplication()) {
        app->setAnimationInterval(1.0 / static_cast<double>(requestedFps));
    }
}

class $modify(ScroFpsMenu, MenuLayer) {
    bool init() override {
        if (!MenuLayer::init()) return false;
        auto& o = options();
        if (o.boost.b) scro::fps::apply();
        return true;
    }
};

class $modify(ScroFpsLevel, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        auto& o = options();
        if (o.boost.b) scro::fps::apply();
        // The tick callback reapplies click-between-steps after each level setup.
    }
};

// ---- Click Precision ---------------------------------------------------------------------
namespace {
struct Raw {
    int button;
    bool down;
    bool player1;
    Clock::time_point time;
};

std::vector<Raw>& rawClicks() {
    static std::vector<Raw> value;
    return value;
}
} // namespace

#ifndef GEODE_IS_ANDROID
bool scro::fps::precise() { return false; }   // the input queue is only held on Android
#else
bool scro::fps::precise() {
    auto& o = options();
    return o.boost.b && o.precision.b && !cbfActive();
}
#endif

bool scro::fps::pending() { return !rawClicks().empty(); }

void scro::fps::hold(int button, bool down, bool player1) {
    auto& queue = rawClicks();
    if (queue.size() >= 128) queue.erase(queue.begin(), queue.begin() + 64);
    queue.push_back({button, down, player1, Clock::now()});
}

std::vector<scro::fps::Held> scro::fps::drain() {
    auto now = Clock::now();
    auto& previous = lastClickTime();
    const float span = std::chrono::duration<float>(now - previous).count();

    std::vector<Held> out;
    auto& queue = rawClicks();
    out.reserve(queue.size());
    for (auto const& click : queue) {
        float fraction = 0.f;
        if (span > 0.f && span < 0.25f) {
            fraction = std::clamp(
                std::chrono::duration<float>(click.time - previous).count() / span,
                0.f, 1.f
            );
        }
        out.push_back({click.button, click.down, click.player1, fraction});
    }
    queue.clear();
    previous = now;
    return out;
}

#ifdef GEODE_IS_ANDROID
class $modify(ScroFpsQueue, GJBaseGameLayer) {
    void queueButton(int button, bool down, bool isPlayer1, double timestamp) {
        if (!scro::fps::g_bypass && PlayLayer::get()) scro::clicks::arrive();
        if (!scro::fps::g_bypass && scro::fps::precise() && PlayLayer::get()) {
            scro::fps::hold(button, down, isPlayer1);
            return;
        }
        GJBaseGameLayer::queueButton(button, down, isPlayer1, timestamp);
    }
};
#endif
