#include "Macro/Bot.hpp"
#include "Labels/Labels.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
using namespace scro;

class $modify(ScroMacroLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& bot = Bot::get();
        if (PlayLayer::get()) {
            if (bot.state == BotState::Playing && !bot.injecting) return;          // lock manual input
            if (down) labels::click();
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
        auto& bot = Bot::get();
        int const frame = m_gameState.m_currentProgress;   // 0 on full restart, checkpoint frame otherwise
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
