#include "Hacks/FrameStep.hpp"
#include "Analysis/FrameCounter.hpp"
#include "Macro/Bot.hpp"
#include "Macro/Editor.hpp"
#include "Utils/Settings.hpp"
#include "Notifications/Notify.hpp"
#include "UI/Pages.hpp"
#include <algorithm>

namespace scro::ui {
using namespace style;

namespace {
std::string g_confirmDelete;     // macro whose delete button was tapped once

PauseLayer* pauseLayer() {
    auto scene = CCDirector::get()->getRunningScene();
    return scene ? scene->getChildByType<PauseLayer>(0) : nullptr;
}

void restartLevel() {
    auto pl = PlayLayer::get();
    if (!pl) return;
    if (auto pause = pauseLayer()) pause->onRestart(nullptr);
    else pl->resetLevel();
}

// Starts or stops recording / playback. Both begin from a fresh start of the level.
void toggleMode(Overlay* o, BotState wanted) {
    auto& bot = Bot::get();
    if (bot.state == wanted) {
        bot.state = BotState::Idle;
        o->refresh();
        return;
    }
    if (!PlayLayer::get()) return scro::notify("Open a level first", false);
    if (wanted == BotState::Playing && bot.actions.empty()) return scro::notify("Load a macro first", false);

    if (wanted == BotState::Recording) {
        bot.actions.clear();
        bot.name.clear();
        bot.assistedRun = false;
        Editor::get().reset();
    } else {
        bot.assistedRun = true;
    }
    bot.state = wanted;
    bot.index = 0;
    o->hide();
    restartLevel();
}

void saveMacro(Overlay* o, bool asNew) {
    auto& bot = Bot::get();
    if (bot.state != BotState::Idle) return scro::notify("Stop the macro first", false);
    if (bot.actions.empty()) return scro::notify("Nothing to save", false);
    std::string name = (asNew || bot.name.empty()) ? bot.suggestName() : bot.name;
    bool ok = bot.save(name);
    if (ok) Editor::get().markSaved();
    scro::notify(ok ? "Saved " + name : "Could not save", ok);
    o->refresh();
}
}

void buildMacro(Page& p, Overlay& overlay) {
    Overlay* o = &overlay;
    auto const& t = theme();
    auto& bot = Bot::get();
    auto pl = PlayLayer::get();
    bool const rec = bot.state == BotState::Recording;
    bool const play = bot.state == BotState::Playing;
    bool const idle = bot.state == BotState::Idle;

    // status
    ccColor4B statusColor = rec ? ccColor4B{255, 110, 110, 255} : play ? ccColor4B{110, 220, 160, 255} : t.text;
    static scro::Option* frameCounter = &scro::find("clickan");
    p.rowLabel(rec ? "Recording" : play ? (frameCounter->b ? "Playing - Frame Counter" : "Playing") : "Idle", statusColor);
    double seconds = static_cast<double>(bot.lastFrame()) / Editor::kStepsPerSecond;
    p.rowLabel(fmt::format("{} inputs  {:.1f}s", bot.actions.size(), seconds), t.muted);
    p.rowLabel(bot.name.empty() ? "not saved" : bot.name, t.muted);

    p.rowToggle("Frame Counter", frameCounter->b, [o] {
        frameCounter->b = !frameCounter->b;
        if (frameCounter->b) scro::fc::reset();
        scro::changed();
        o->setInfo("Frame Counter: the macro reads the frame window of every click while it plays.");
        o->refresh();
    });
    p.rowHeading("Record");
    p.rowCells({
        {rec ? "Stop" : "Record", rec ? Btn::Danger : Btn::Accent, [o] { toggleMode(o, BotState::Recording); }},
        {play ? "Stop" : "Play", play ? Btn::Danger : Btn::Normal, [o] { toggleMode(o, BotState::Playing); }},
    });
    bool have = !bot.actions.empty() && idle;
    p.rowCells({
        {"Save", Btn::Normal, have ? std::function<void()>([o] { saveMacro(o, false); }) : std::function<void()>()},
        {"Save New", Btn::Normal, have ? std::function<void()>([o] { saveMacro(o, true); }) : std::function<void()>()},
        {"Clear", Btn::Normal, have ? std::function<void()>([o] {
            auto& b = Bot::get();
            b.actions.clear();
            b.name.clear();
            Editor::get().reset();
            o->refresh();
        }) : std::function<void()>()},
    });

    // controls that only make sense inside a level
    if (pl) {
        p.rowHeading("Level");
        bool paused = pauseLayer() != nullptr;
        p.rowCells({
            {paused ? "Resume" : "Pause", Btn::Normal, [o] {
                auto level = PlayLayer::get();
                if (!level) return;
                if (auto pause = pauseLayer()) pause->onResume(nullptr);
                else level->pauseGame(false);
                o->refresh();
            }},
            {"Restart", Btn::Normal, [] { restartLevel(); }},
        });
        if (scro::stepper::frozen()) p.rowAction("Step", Btn::Accent, [] { scro::stepper::request(); });
        if (pl->m_isPracticeMode) {
            p.rowCells({
                {"Check +", Btn::Normal, [] {
                    if (auto level = PlayLayer::get(); level && level->m_uiLayer) level->m_uiLayer->onCheck(nullptr);
                }},
                {"Check -", Btn::Normal, [] {
                    if (auto level = PlayLayer::get(); level && level->m_uiLayer) level->m_uiLayer->onDeleteCheck(nullptr);
                }},
            });
        }
    }

    p.rowHeading("Saved");
    auto saved = bot.list();
    if (saved.empty()) p.rowLabel("No saved macros", t.muted);
    float const cellW = rowH() * 1.9f;
    for (auto const& info : saved) {
        std::string name = info.name;
        bool confirm = g_confirmDelete == name;
        p.rowWithCells(fmt::format("{}{}  {}", name == bot.name ? "> " : "", name, info.inputs),
            [o, name, idle] {
                if (!idle) return scro::notify("Stop the macro first", false);
                bool ok = Bot::get().load(name);
                if (ok) Editor::get().reset();
                scro::notify(ok ? "Loaded " + name : "Not a valid macro", ok);
                o->refresh();
            },
            {
                {"Edit", Btn::Normal, [o, name, idle] {
                    if (!idle) return scro::notify("Stop the macro first", false);
                    if (!Bot::get().load(name)) return scro::notify("Not a valid macro", false);
                    Editor::get().reset();
                    o->openWindow("Editor");
                }},
                {confirm ? "Sure?" : "Del", confirm ? Btn::Danger : Btn::Quiet, [o, name, confirm] {
                    if (!confirm) { g_confirmDelete = name; o->refresh(); return; }
                    g_confirmDelete.clear();
                    bool ok = Bot::get().remove(name);
                    scro::notify(ok ? "Deleted " + name : "Could not delete", ok);
                    o->refresh();
                }},
            },
            cellW);
    }
}
}
