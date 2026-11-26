#include "Hacks/FpsBoost.hpp"
#include "Macro/Bot.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

static auto& safe = scro::toggle("Global", "safe", "Safe Mode", "Completions count as test runs. Cheat-like options (No Clip, Jump Hack, Speedhack, Boost Frames, macros) always send a test run, whatever this says.", true, 25);
static bool const safeSection = (safe.section = "Safety", true);   // never locked: you can switch it freely

// Anything cheat-like forces a test run so nothing is ever saved or submitted.
static bool forced() {
    static auto* noclip = &scro::find("noclip");
    static auto* speed = &scro::find("speed");
    static auto* jump = &scro::find("jump");
    static auto* freeze = &scro::find("freeze");
    static auto* fstep = &scro::find("framestep");
    return scro::fps::boostFrames() || noclip->b || jump->b || freeze->b || fstep->b || speed->f != 1.f ||
           scro::Bot::get().state == scro::BotState::Playing || scro::Bot::get().assistedRun;
}

class $modify(ScroSafeMode, PlayLayer) {
    void levelComplete() {
        if (safe.b || forced()) m_isTestMode = true;
        PlayLayer::levelComplete();
    }
};
