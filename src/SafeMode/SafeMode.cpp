#include "Macro/Bot.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

static auto& safe = scro::toggle("Global", "safe", "Safe Mode", "Completions count as test runs.", true, 25);

// Anything cheat-like forces a test run so nothing is ever saved or submitted.
static bool forced() {
    static auto* noclip = &scro::find("noclip");
    static auto* speed = &scro::find("speed");
    static auto* jump = &scro::find("jump");
    static auto* freeze = &scro::find("freeze");
    static auto* fstep = &scro::find("framestep");
    return noclip->b || jump->b || freeze->b || fstep->b || speed->f != 1.f ||
           scro::Bot::get().state == scro::BotState::Playing;
}

class $modify(ScroSafeMode, PlayLayer) {
    void levelComplete() {
        if (safe.b || forced()) m_isTestMode = true;
        PlayLayer::levelComplete();
    }
};
