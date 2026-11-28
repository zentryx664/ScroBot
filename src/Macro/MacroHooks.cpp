#include "Macro/Bot.hpp"
#include "Analysis/FrameCounter.hpp"
#include "Labels/Labels.hpp"
#include "Utils/Settings.hpp"
#include <algorithm>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
using namespace scro;

static scro::Option& fixClick = scro::toggle("Global", "fixclick", "Fix Click",
    "Blocks double clicks: the same press or release can only happen once per tick (physics step).", false, 40);
static bool const fixClickSection = (fixClick.section = "Misc", true);
namespace {
int g_lastProgress = -1, g_lastKey = -1;      // the last input that was let through
int g_curFrame = -1, g_prevFrame = -1;        // live play: the frames of the last two different inputs

// Frame window of an input: how many frames of room it has before the neighbouring inputs.
// With a macro playing, both neighbours are known; live, only the previous one is.
int estimateWindow(Bot& bot, int frame) {
    constexpr int kMax = 12;
    if (bot.state == BotState::Playing && bot.injecting && bot.index > 0) {
        auto const& acts = bot.actions;
        long i = static_cast<long>(bot.index) - 1;
        int f = acts[static_cast<size_t>(i)].frame;
        long j = i - 1;
        while (j >= 0 && acts[static_cast<size_t>(j)].frame == f) --j;
        long k = i + 1;
        while (k < static_cast<long>(acts.size()) && acts[static_cast<size_t>(k)].frame == f) ++k;
        int prevGap = j >= 0 ? f - acts[static_cast<size_t>(j)].frame : kMax;
        int nextGap = k < static_cast<long>(acts.size()) ? acts[static_cast<size_t>(k)].frame - f : kMax;
        return std::clamp(std::min(prevGap, nextGap), 1, kMax);
    }
    if (frame != g_curFrame) { g_prevFrame = g_curFrame; g_curFrame = frame; }
    return g_prevFrame < 0 ? kMax : std::clamp(frame - g_prevFrame, 1, kMax);
}
}

class $modify(ScroMacroLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& bot = Bot::get();
        if (PlayLayer::get()) {
            if (bot.state == BotState::Playing && !bot.injecting) return;          // lock manual input
            if (fixClick.b && !bot.injecting) {
                int const key = (down ? 1 : 0) | (button << 1) | (isPlayer1 ? 64 : 0);
                int const progress = m_gameState.m_currentProgress;
                if (progress == g_lastProgress && key == g_lastKey) return;       // same input twice in one tick
                g_lastProgress = progress;
                g_lastKey = key;
            }
            if (down) labels::click();
            if (scro::fc::enabled()) {
                int const window = estimateWindow(bot, m_gameState.m_currentProgress);
                if (down) scro::fc::pressed(window, m_player1);
            }
            if (bot.state == BotState::Recording && m_player1 && !m_player1->m_isDead) {
                Action a{};
                a.frame = m_gameState.m_currentProgress;
                a.button = button; a.down = down; a.p1 = isPlayer1; a.hasState = 1;
                a.a = grab(m_player1); a.b = grab(m_player2);
                bot.actions.push_back(a);
            }
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        auto& bot = Bot::get();
        if (PlayLayer::get()) scro::fc::step(isHalfTick);
        if (bot.state == BotState::Playing && PlayLayer::get()) {
            int const frame = m_gameState.m_currentProgress;
            while (bot.index < bot.actions.size() && bot.actions[bot.index].frame <= frame) {
                auto const& a = bot.actions[bot.index++];
                if (a.hasState) {
                    apply(m_player1, a.a);
                    if (m_gameState.m_isDualMode) apply(m_player2, a.b);
                }
                bot.injecting = true;
                GJBaseGameLayer::handleButton(a.down, a.button, a.p1);
                bot.injecting = false;
            }
            // End playback cleanly, but retain assistedRun until a fresh run/quit so
            // manual continuation cannot be mistaken for a clean completion.
            if (bot.index >= bot.actions.size()) bot.state = BotState::Idle;
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

class $modify(ScroMacroPlay, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        g_lastProgress = g_lastKey = -1;
        g_curFrame = g_prevFrame = -1;
        auto& bot = Bot::get();
        int const frame = m_gameState.m_currentProgress;   // 0 on full restart, checkpoint frame otherwise
        if (frame == 0) scro::fc::reset();                 // a fresh run starts a fresh count
        if (bot.state == BotState::Recording) {
            while (!bot.actions.empty() && bot.actions.back().frame >= frame) bot.actions.pop_back();
        } else if (bot.state == BotState::Playing) {
            bot.assistedRun = true;
            this->m_isTestMode = true;
            bot.seek(frame);
        } else if (frame == 0) {
            bot.assistedRun = false;
        }
    }
    void onQuit() {
        Bot::get().state = BotState::Idle;
        Bot::get().assistedRun = false;
        PlayLayer::onQuit();
    }
};
