#include "UI/MacroView.hpp"
#include "Macro/Bot.hpp"
#include "Notifications/Notify.hpp"
#include "UI/Theme.hpp"
#include "Utils/Draw.hpp"
#include <algorithm>
using namespace scro;
using namespace scro::draw;

MacroView* MacroView::create(float w, bool desk, std::function<void()> onStart) {
    auto r = new MacroView();
    if (r->init() && r->setup(w, desk, std::move(onStart))) { r->autorelease(); return r; }
    delete r;
    return nullptr;
}
bool MacroView::setup(float w, bool desk, std::function<void()> onStart) {
    m_w = w; m_d = desk; m_onStart = std::move(onStart);
    m_h = desk ? 240.f : 190.f;
    this->setContentSize({w, m_h});
    rebuild();
    return true;
}

void MacroView::rebuild() {
    this->removeAllChildren();
    m_input = nullptr;
    m_names.clear();
    std::error_code ec;
    for (auto& e : std::filesystem::directory_iterator(Bot::dir(), ec))
        if (e.path().extension() == ".scro") m_names.push_back(e.path().stem().string());
    std::sort(m_names.begin(), m_names.end());

    auto& bot = Bot::get();
    bool rec = bot.state == BotState::Recording, play = bot.state == BotState::Playing;
    float mg = m_d ? 6.f : 12.f, gap = 4.f, sth = m_d ? 16.f : 22.f, bh = m_d ? 20.f : 26.f, rowH = m_d ? 17.f : 24.f;
    float fs = m_d ? 0.62f : 0.8f, inner = m_w - mg * 2, y = m_h - 4.f;
    auto ac = theme::accent();

    auto menu = CCMenu::create();
    menu->setPosition(0, 0);
    menu->setTouchPriority(-510);
    this->addChild(menu);

    // status: dot + text
    auto dot = rect(8, 8, rec ? ccc4(229, 72, 77, 255) : play ? ccc4(76, 195, 138, 255) : ccc4(70, 70, 74, 255));
    dot->setPosition(mg, y - sth / 2 - 4); this->addChild(dot);
    std::string status = rec ? "Recording" : play ? "Playing" : bot.assistedRun ? "SHOWCASE - MACRO ASSISTED" : "Idle";
    this->addChild(label(status, fs, bot.assistedRun ? ccColor3B{255, 190, 95} : theme::TEXT,
        {0.f, 0.5f}, {mg + 14, y - sth / 2}));
    this->addChild(label(fmt::format("{} inputs", bot.actions.size()), fs * 0.9f, theme::MUTED, {1.f, 0.5f}, {m_w - mg, y - sth / 2}));
    y -= sth + gap;

    // Record | Play
    float bw = (inner - gap) / 2;
    auto b1 = CCMenuItemSpriteExtra::create(button(rec ? "Stop" : "Record", bw, bh), this, menu_selector(MacroView::onRecord));
    b1->m_scaleMultiplier = 1.f; b1->setPosition(mg + bw / 2, y - bh / 2);
    auto b2 = CCMenuItemSpriteExtra::create(button(play ? "Stop" : "Play", bw, bh), this, menu_selector(MacroView::onPlay));
    b2->m_scaleMultiplier = 1.f; b2->setPosition(mg + bw + gap + bw / 2, y - bh / 2);
    menu->addChild(b1); menu->addChild(b2);
    y -= bh + gap;

    // name + Save
    float sw = m_d ? 36.f : 50.f, iw = inner - sw - gap;
    m_input = TextInput::create(iw, "Name", "chatFont.fnt");
    m_input->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-");
    m_input->setString(m_sel >= 0 && m_sel < static_cast<int>(m_names.size()) ? m_names[m_sel] : "macro1");
    m_input->setPosition(mg + iw / 2, y - bh / 2);
    this->addChild(m_input);
    auto sv = CCMenuItemSpriteExtra::create(button("Save", sw, bh), this, menu_selector(MacroView::onSave));
    sv->m_scaleMultiplier = 1.f; sv->setPosition(m_w - mg - sw / 2, y - bh / 2);
    menu->addChild(sv);
    y -= bh + gap;

    // saved macros (3 per page)
    int pages = std::max(1, static_cast<int>((m_names.size() + 2) / 3));
    m_page = std::min(m_page, pages - 1);
    if (m_names.empty()) this->addChild(label("No saved macros", fs, theme::MUTED, {0.5f, 0.5f}, {m_w / 2, y - rowH * 1.5f}));
    for (int i = 0; i < 3; i++) {
        int idx = m_page * 3 + i;
        if (idx >= static_cast<int>(m_names.size())) break;
        bool sel = idx == m_sel;
        auto n = CCNode::create();
        n->setContentSize({inner, rowH});
        n->addChild(rect(inner, rowH, sel ? ccc4(26, 26, 28, 255) : ccc4(19, 19, 21, 255)));
        auto sb = rect(2, rowH * 0.6f, sel ? ccc4(ac.r, ac.g, ac.b, 255) : ccc4(60, 60, 64, 255));
        sb->setPosition(4, rowH * 0.2f);
        n->addChild(sb);
        n->addChild(label(m_names[idx], fs, theme::TEXT, {0.f, 0.5f}, {12, rowH / 2}));
        auto it = CCMenuItemSpriteExtra::create(n, this, menu_selector(MacroView::onPick));
        it->m_scaleMultiplier = 1.f; it->setTag(idx);
        it->setPosition(m_w / 2, y - rowH / 2 - i * (rowH + 2));
        menu->addChild(it);
    }
    y -= 3 * (rowH + 2) + gap;
    if (pages > 1) {
        auto pv = CCMenuItemSpriteExtra::create(button("<", 22, 14), this, menu_selector(MacroView::onPage));
        pv->setTag(-1); pv->setPosition(m_w - mg - 11 - 52, y - 7);
        auto nx = CCMenuItemSpriteExtra::create(button(">", 22, 14), this, menu_selector(MacroView::onPage));
        nx->setTag(1); nx->setPosition(m_w - mg - 11, y - 7);
        menu->addChild(pv); menu->addChild(nx);
        this->addChild(label(fmt::format("{}/{}", m_page + 1, pages), fs, theme::MUTED, {0.5f, 0.5f}, {m_w - mg - 32, y - 7}));
    }
}

void MacroView::begin(int st) {
    auto& b = Bot::get();
    auto state = static_cast<BotState>(st);
    if (b.state == state) { b.state = BotState::Idle; rebuild(); return; }
    if (state == BotState::Recording) b.assistedRun = false;
    if (state == BotState::Playing) b.assistedRun = true;
    PauseLayer* pause = nullptr;
    if (auto scene = CCDirector::get()->getRunningScene()) pause = scene->getChildByType<PauseLayer>(0);
    b.state = state;
    if (state == BotState::Recording) b.actions.clear();
    b.index = 0;
    if (m_onStart) m_onStart();
    if (pause) pause->onRestart(nullptr);
    else if (auto pl = PlayLayer::get()) pl->resetLevel();
}
void MacroView::onRecord(CCObject*) { begin(static_cast<int>(BotState::Recording)); }
void MacroView::onPlay(CCObject*) {
    auto& bot = Bot::get();
    if (bot.state == BotState::Playing) { begin(static_cast<int>(BotState::Playing)); return; }
    if (bot.actions.empty()) return notify("Load a macro first", false);
    FLAlertLayer::create(this, "Macro Playback Warning", "Cancel", "Continue",
        "Macros are for fun, practice, testing and level showcases. This run will be marked macro-assisted, and completion progress will be restricted where the game supports it. Do not present it as a legitimate completion.", 320.f)->show();
}
void MacroView::FLAlert_Clicked(FLAlertLayer*, bool btn2) {
    if (btn2 && !Bot::get().actions.empty()) begin(static_cast<int>(BotState::Playing));
}
void MacroView::onSave(CCObject*) {
    bool ok = m_input && Bot::get().save(m_input->getString());
    notify(ok ? "Macro saved" : "Nothing to save", ok);
    if (ok) rebuild();
}
void MacroView::onPick(CCObject* s) {
    int idx = static_cast<CCNode*>(s)->getTag();
    if (idx < 0 || idx >= static_cast<int>(m_names.size())) return;
    bool ok = Bot::get().load(m_names[idx]);
    m_sel = idx;
    notify(ok ? "Macro loaded" : "Invalid macro file", ok);
    rebuild();
}
void MacroView::onPage(CCObject* s) {
    m_page = std::max(0, m_page + static_cast<CCNode*>(s)->getTag());
    rebuild();
}
