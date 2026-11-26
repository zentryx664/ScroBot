#include "Macro/Editor.hpp"
#include <algorithm>
#include <numeric>

namespace scro {
Editor& Editor::get() { static Editor e; return e; }

void Editor::reset() {
    sel.clear();
    m_undo.clear();
    m_redo.clear();
    page = 0;
    dirty = false;
}

void Editor::sync() {
    auto& a = Bot::get().actions;
    if (sel.size() != a.size()) sel.assign(a.size(), 0);
}

size_t Editor::selectedCount() {
    sync();
    return static_cast<size_t>(std::count(sel.begin(), sel.end(), 1));
}

int Editor::firstSelectedFrame() {
    sync();
    auto& a = Bot::get().actions;
    for (size_t i = 0; i < a.size(); ++i) if (sel[i]) return a[i].frame;
    return 0;
}

int Editor::lastSelectedFrame() {
    sync();
    auto& a = Bot::get().actions;
    for (size_t i = a.size(); i-- > 0;) if (sel[i]) return a[i].frame;
    return 0;
}

void Editor::selectAll(bool value) {
    sync();
    std::fill(sel.begin(), sel.end(), value ? 1 : 0);
}

void Editor::toggle(size_t index) {
    sync();
    if (index < sel.size()) sel[index] = sel[index] ? 0 : 1;
}

void Editor::snapshot() {
    m_undo.push_back(Bot::get().actions);
    if (m_undo.size() > 40) m_undo.erase(m_undo.begin());
    m_redo.clear();
    dirty = true;
}

void Editor::undo() {
    if (m_undo.empty()) return;
    auto& bot = Bot::get();
    m_redo.push_back(bot.actions);
    bot.actions = std::move(m_undo.back());
    m_undo.pop_back();
    bot.index = 0;
    sel.assign(bot.actions.size(), 0);
    dirty = true;
}

void Editor::redo() {
    if (m_redo.empty()) return;
    auto& bot = Bot::get();
    m_undo.push_back(bot.actions);
    bot.actions = std::move(m_redo.back());
    m_redo.pop_back();
    bot.index = 0;
    sel.assign(bot.actions.size(), 0);
    dirty = true;
}

void Editor::normalize() {
    auto& bot = Bot::get();
    auto& a = bot.actions;
    std::vector<size_t> order(a.size());
    std::iota(order.begin(), order.end(), size_t{0});
    std::stable_sort(order.begin(), order.end(), [&](size_t x, size_t y) { return a[x].frame < a[y].frame; });
    std::vector<Action> na;
    std::vector<char> ns;
    na.reserve(a.size());
    ns.reserve(a.size());
    for (auto i : order) { na.push_back(a[i]); ns.push_back(sel[i]); }
    a = std::move(na);
    sel = std::move(ns);
    bot.index = 0;
}

void Editor::remove() {
    sync();
    if (!selectedCount()) return;
    snapshot();
    auto& a = Bot::get().actions;
    std::vector<Action> keep;
    for (size_t i = 0; i < a.size(); ++i) if (!sel[i]) keep.push_back(a[i]);
    a = std::move(keep);
    sel.assign(a.size(), 0);
    Bot::get().index = 0;
}

void Editor::nudge(int frames) {
    sync();
    if (!selectedCount()) return;
    snapshot();
    auto& a = Bot::get().actions;
    for (size_t i = 0; i < a.size(); ++i) {
        if (!sel[i]) continue;
        a[i].frame = std::max(0, a[i].frame + frames);
        a[i].hasState = 0;
    }
    normalize();
}

void Editor::flip() {
    sync();
    if (!selectedCount()) return;
    snapshot();
    auto& a = Bot::get().actions;
    for (size_t i = 0; i < a.size(); ++i) if (sel[i]) { a[i].down = a[i].down ? 0 : 1; a[i].hasState = 0; }
}

void Editor::swapPlayer() {
    sync();
    if (!selectedCount()) return;
    snapshot();
    auto& a = Bot::get().actions;
    for (size_t i = 0; i < a.size(); ++i) if (sel[i]) { a[i].p1 = a[i].p1 ? 0 : 1; a[i].hasState = 0; }
}

void Editor::duplicate() {
    sync();
    if (!selectedCount()) return;
    snapshot();
    auto& a = Bot::get().actions;
    size_t n = a.size();
    for (size_t i = 0; i < n; ++i) {
        if (!sel[i]) continue;
        Action copy = a[i];
        copy.frame += 4;
        copy.hasState = 0;
        a.push_back(copy);
        sel.push_back(1);
        sel[i] = 0;
    }
    normalize();
}

void Editor::addClick(int frame) {
    sync();
    snapshot();
    auto& a = Bot::get().actions;
    std::fill(sel.begin(), sel.end(), 0);
    Action press{};
    press.frame = std::max(0, frame);
    press.button = 1;
    press.down = 1;
    press.p1 = 1;
    Action release = press;
    release.frame = press.frame + 3;
    release.down = 0;
    a.push_back(press);
    a.push_back(release);
    sel.push_back(1);
    sel.push_back(1);
    normalize();
}

void Editor::trimBefore() {
    sync();
    if (!selectedCount()) return;
    int from = firstSelectedFrame();
    snapshot();
    auto& a = Bot::get().actions;
    std::vector<Action> keep;
    std::vector<char> keepSel;
    for (size_t i = 0; i < a.size(); ++i) if (a[i].frame >= from) { keep.push_back(a[i]); keepSel.push_back(sel[i]); }
    a = std::move(keep);
    sel = std::move(keepSel);
    Bot::get().index = 0;
}

void Editor::trimAfter() {
    sync();
    if (!selectedCount()) return;
    int to = lastSelectedFrame();
    snapshot();
    auto& a = Bot::get().actions;
    std::vector<Action> keep;
    std::vector<char> keepSel;
    for (size_t i = 0; i < a.size(); ++i) if (a[i].frame <= to) { keep.push_back(a[i]); keepSel.push_back(sel[i]); }
    a = std::move(keep);
    sel = std::move(keepSel);
    Bot::get().index = 0;
}

void Editor::shiftAll(int frames) {
    sync();
    auto& a = Bot::get().actions;
    if (a.empty()) return;
    snapshot();
    for (auto& x : a) {
        x.frame = std::max(0, x.frame + frames);
        x.hasState = 0;
    }
    normalize();
}

void Editor::stripPositions() {
    auto& a = Bot::get().actions;
    if (a.empty()) return;
    snapshot();
    for (auto& x : a) x.hasState = 0;
}
}
