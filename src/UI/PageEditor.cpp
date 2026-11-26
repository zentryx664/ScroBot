#include "Macro/Bot.hpp"
#include "Macro/Editor.hpp"
#include "Notifications/Notify.hpp"
#include "UI/Pages.hpp"
#include <algorithm>

namespace scro::ui {
using namespace style;

namespace {
constexpr int kRowsPerPage = 20;

char const* buttonName(int button) {
    switch (button) {
        case 1: return "Jump";
        case 2: return "Left";
        case 3: return "Right";
        default: return "Other";
    }
}

// Wraps an edit so it only runs while the bot is idle, then redraws the windows.
std::function<void()> guarded(Overlay* o, std::function<void()> edit) {
    return [o, edit = std::move(edit)] {
        if (Bot::get().state != BotState::Idle) return scro::notify("Stop the macro first", false);
        edit();
        o->refresh();
    };
}
}

void buildEditor(Page& p, Overlay& overlay) {
    Overlay* o = &overlay;
    auto const& t = theme();
    auto& bot = Bot::get();
    auto& ed = Editor::get();
    ed.sync();

    if (bot.actions.empty()) {
        p.rowLabel("No macro loaded", t.muted);
        p.rowLabel("Record one, or press Edit", t.muted);
        p.rowLabel("on a saved macro", t.muted);
        return;
    }
    bool const busy = bot.state != BotState::Idle;
    bool const any = ed.selectedCount() > 0 && !busy;

    std::string title = bot.name.empty() ? "Unsaved macro" : bot.name;
    if (ed.dirty) title += " (edited)";
    p.rowLabel(title, t.text);
    p.rowLabel(fmt::format("{} inputs  step {}  {:.1f}s  {} selected", bot.actions.size(), bot.lastFrame(),
        static_cast<double>(bot.lastFrame()) / Editor::kStepsPerSecond, ed.selectedCount()), t.muted);
    if (busy) p.rowLabel("Stop recording or playback to edit", ccColor4B{255, 190, 95, 255});

    // each cell only works when it can: edits need a selection and an idle bot
    auto sel = [&](Btn kind, std::function<void()> fn) {
        return Cell{"", kind, any ? guarded(o, std::move(fn)) : std::function<void()>()};
    };
    auto whole = [&](std::function<void()> fn) {
        return busy ? std::function<void()>() : guarded(o, std::move(fn));
    };

    p.rowHeading("Select");
    p.rowCells({
        {"All", Btn::Normal, [o] { Editor::get().selectAll(true); o->refresh(); }},
        {"None", Btn::Normal, [o] { Editor::get().selectAll(false); o->refresh(); }},
    });

    p.rowHeading("Selected inputs");
    {
        Cell a = sel(Btn::Normal, [] { Editor::get().nudge(-10); }); a.text = "-10";
        Cell b = sel(Btn::Normal, [] { Editor::get().nudge(-1); }); b.text = "-1";
        Cell c = sel(Btn::Normal, [] { Editor::get().nudge(1); }); c.text = "+1";
        Cell d = sel(Btn::Normal, [] { Editor::get().nudge(10); }); d.text = "+10";
        p.rowCells({a, b, c, d});
    }
    {
        Cell a = sel(Btn::Normal, [] { Editor::get().flip(); }); a.text = "Press/Release";
        Cell b = sel(Btn::Normal, [] { Editor::get().swapPlayer(); }); b.text = "Swap Player";
        Cell c = sel(Btn::Normal, [] { Editor::get().duplicate(); }); c.text = "Duplicate";
        p.rowCells({a, b, c});
    }
    {
        Cell a = sel(Btn::Normal, [] { Editor::get().trimBefore(); }); a.text = "Trim Before";
        Cell b = sel(Btn::Normal, [] { Editor::get().trimAfter(); }); b.text = "Trim After";
        Cell c = sel(Btn::Danger, [] { Editor::get().remove(); }); c.text = "Delete";
        p.rowCells({a, b, c});
    }

    p.rowHeading("Whole macro");
    p.rowCells({
        {"-10", Btn::Normal, whole([] { Editor::get().shiftAll(-10); })},
        {"-1", Btn::Normal, whole([] { Editor::get().shiftAll(-1); })},
        {"+1", Btn::Normal, whole([] { Editor::get().shiftAll(1); })},
        {"+10", Btn::Normal, whole([] { Editor::get().shiftAll(10); })},
    });
    p.rowCells({
        {"Add Click", Btn::Normal, whole([] {
            auto& e = Editor::get();
            int at = e.selectedCount() ? e.lastSelectedFrame() + 10 : Bot::get().lastFrame() + 10;
            e.addClick(at);
        })},
        {"Strip Positions", Btn::Normal, whole([] { Editor::get().stripPositions(); })},
    });

    p.rowHeading("History");
    {
        bool u = ed.canUndo() && !busy, r = ed.canRedo() && !busy;
        auto save = [o](bool asNew) {
            return [o, asNew] {
                auto& b = Bot::get();
                std::string name = (asNew || b.name.empty()) ? b.suggestName() : b.name;
                bool ok = b.save(name);
                if (ok) Editor::get().markSaved();
                scro::notify(ok ? "Saved " + name : "Could not save", ok);
                o->refresh();
            };
        };
        p.rowCells({
            {"Undo", Btn::Normal, u ? guarded(o, [] { Editor::get().undo(); }) : std::function<void()>()},
            {"Redo", Btn::Normal, r ? guarded(o, [] { Editor::get().redo(); }) : std::function<void()>()},
            {"Save", Btn::Accent, busy ? std::function<void()>() : std::function<void()>(save(false))},
            {"Save New", Btn::Normal, busy ? std::function<void()>() : std::function<void()>(save(true))},
        });
    }

    // input list, one page at a time
    int const total = static_cast<int>(bot.actions.size());
    int const pages = std::max(1, (total + kRowsPerPage - 1) / kRowsPerPage);
    ed.page = std::clamp(ed.page, 0, pages - 1);
    p.rowHeading(fmt::format("Inputs  page {}/{}", ed.page + 1, pages));
    p.rowCells({
        {"<", Btn::Normal, ed.page > 0 ? std::function<void()>([o] { Editor::get().page--; o->refresh(); }) : std::function<void()>()},
        {"To Selection", Btn::Normal, [o] {
            auto& e = Editor::get();
            e.sync();
            for (size_t i = 0; i < e.sel.size(); ++i) if (e.sel[i]) { e.page = static_cast<int>(i) / kRowsPerPage; break; }
            o->refresh();
        }},
        {">", Btn::Normal, ed.page + 1 < pages ? std::function<void()>([o] { Editor::get().page++; o->refresh(); }) : std::function<void()>()},
    });

    float const w = p.width();
    float const cIndex = kSize.pad * 2.f + kSize.row;
    float const cStep = cIndex + w * 0.13f;
    float const cButton = cStep + w * 0.2f;
    float const cType = cButton + w * 0.15f;
    float const cPlayer = cType + w * 0.17f;
    int const from = ed.page * kRowsPerPage;
    int const to = std::min(total, from + kRowsPerPage);
    for (int i = from; i < to; ++i) {
        auto const& a = bot.actions[static_cast<size_t>(i)];
        bool picked = ed.sel[static_cast<size_t>(i)] != 0;
        float y = p.y();
        float h = kSize.row;
        p.box(0.f, y, w, h, t.card);
        if (picked) p.box(0.f, y, w, h, withAlpha(t.accent, 70));
        float mark = h * 0.42f;
        p.box(kSize.pad, y + (h - mark) / 2.f, mark, mark, picked ? t.accent : t.off);
        p.text(fmt::format("{}", i + 1), t.muted, cIndex, y, h);
        p.text(fmt::format("{}", a.frame), t.text, cStep, y, h);
        p.text(buttonName(a.button), t.text, cButton, y, h);
        p.text(a.down ? "Press" : "Release", a.down ? t.accent : t.muted, cType, y, h);
        p.text(a.p1 ? "P1" : "P2", t.muted, cPlayer, y, h);
        if (a.hasState) p.text("pos", t.muted, w - kSize.pad, y, h, 1.f);
        p.tap(0.f, y, w, h, [o, i] { Editor::get().toggle(static_cast<size_t>(i)); o->refresh(); });
        p.skip(h + kSize.gap);
    }
}
}
