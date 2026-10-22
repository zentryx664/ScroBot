#include "UI/FindView.hpp"
#include "UI/Theme.hpp"
#include "Utils/Draw.hpp"
#include <algorithm>
#include <cctype>
using namespace scro;
using namespace scro::draw;

namespace {
std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
}

FindView* FindView::create(float w, float h, std::function<void(Option*)> onSelect) {
    auto r = new FindView();
    if (r->init() && r->setup(w, h, std::move(onSelect))) { r->autorelease(); return r; }
    delete r;
    return nullptr;
}

bool FindView::setup(float w, float h, std::function<void(Option*)> onSelect) {
    m_w = w; m_h = h; m_onSelect = std::move(onSelect);
    this->setContentSize({w, h});
    rebuild();
    return true;
}

void FindView::runSearch() {
    m_results.clear();
    auto q = lower(m_query);
    if (q.empty()) { m_page = 0; return; }
    for (auto* o : allOptions()) {
        std::string haystack = lower(o->name + " " + o->desc + " " + o->cat + " " + o->section + " " + o->id);
        if (haystack.find(q) != std::string::npos) m_results.push_back(o);
    }
    m_page = 0;
}

void FindView::rebuild() {
    this->removeAllChildren();
    m_input = nullptr;
    m_visible.clear();
    auto menu = CCMenu::create();
    menu->setPosition(0, 0);
    menu->setTouchPriority(-510);
    this->addChild(menu);

    float inputY = m_h - 13.f;
    float searchW = 48.f;
    float inputW = std::max(70.f, m_w - searchW - 6.f);
    m_input = TextInput::create(inputW, "Search name, category, description...", "chatFont.fnt");
    m_input->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-.");
    m_input->setString(m_query);
    m_input->setPosition(inputW / 2, inputY);
    this->addChild(m_input);
    auto search = CCMenuItemSpriteExtra::create(button("Find", searchW, 22.f), this, menu_selector(FindView::onSearch));
    search->m_scaleMultiplier = 1.f;
    search->setPosition(inputW + 6.f + searchW / 2, inputY);
    menu->addChild(search);

    this->addChild(label("Search Scro features by name, category or description", 0.52f, theme::MUTED,
        {0.f, 0.5f}, {2.f, m_h - 31.f}));

    constexpr int pageSize = 5;
    int pages = std::max(1, static_cast<int>((m_results.size() + pageSize - 1) / pageSize));
    m_page = std::clamp(m_page, 0, pages - 1);
    float rowH = 22.f;
    float firstY = m_h - 48.f;
    for (int i = 0; i < pageSize; ++i) {
        int idx = m_page * pageSize + i;
        if (idx >= static_cast<int>(m_results.size())) break;
        auto* o = m_results[idx];
        m_visible.push_back(o);
        auto row = CCNode::create();
        row->setContentSize({m_w, rowH});
        row->addChild(rect(m_w, rowH - 1.f, ccc4(25, 25, 29, 255)));
        auto name = label(o->name, 0.62f, theme::TEXT, {0.f, 0.5f}, {5.f, rowH / 2});
        name->limitLabelWidth(m_w * 0.56f, 0.62f, 0.28f);
        row->addChild(name);
        std::string meta = o->cat + " / " + o->section;
        auto category = label(meta, 0.48f, theme::MUTED, {1.f, 0.5f}, {m_w - 5.f, rowH / 2});
        category->limitLabelWidth(m_w * 0.42f, 0.48f, 0.25f);
        row->addChild(category);
        auto item = CCMenuItemSpriteExtra::create(row, this, menu_selector(FindView::onResult));
        item->m_scaleMultiplier = 1.f;
        item->setTag(static_cast<int>(m_visible.size()) - 1);
        item->setPosition(m_w / 2, firstY - i * rowH - rowH / 2);
        menu->addChild(item);
    }

    if (m_query.empty()) {
        this->addChild(label("Enter a search and press Find.", 0.65f, theme::MUTED,
            {0.5f, 0.5f}, {m_w / 2, m_h / 2 - 4.f}));
    } else if (m_results.empty()) {
        this->addChild(label("No matching features. Try a shorter search.", 0.58f, theme::MUTED,
            {0.5f, 0.5f}, {m_w / 2, m_h / 2 - 4.f}));
    } else {
        this->addChild(label(fmt::format("{} result(s)", m_results.size()), 0.52f, theme::MUTED,
            {0.f, 0.5f}, {2.f, 8.f}));
        if (pages > 1) {
            auto prev = CCMenuItemSpriteExtra::create(button("<", 22.f, 15.f), this, menu_selector(FindView::onPage));
            prev->setTag(-1); prev->setPosition(m_w - 49.f, 8.f); prev->setEnabled(m_page > 0); menu->addChild(prev);
            auto next = CCMenuItemSpriteExtra::create(button(">", 22.f, 15.f), this, menu_selector(FindView::onPage));
            next->setTag(1); next->setPosition(m_w - 13.f, 8.f); next->setEnabled(m_page + 1 < pages); menu->addChild(next);
            this->addChild(label(fmt::format("{}/{}", m_page + 1, pages), 0.5f, theme::MUTED,
                {1.f, 0.5f}, {m_w - 56.f, 8.f}));
        }
    }
}

void FindView::onSearch(CCObject*) {
    m_query = m_input ? m_input->getString() : m_query;
    runSearch();
    rebuild();
}
void FindView::onPage(CCObject* s) {
    m_page += static_cast<CCNode*>(s)->getTag();
    rebuild();
}
void FindView::onResult(CCObject* s) {
    int idx = static_cast<CCNode*>(s)->getTag();
    if (idx < 0 || idx >= static_cast<int>(m_visible.size())) return;
    if (m_onSelect) m_onSelect(m_visible[idx]);
}
