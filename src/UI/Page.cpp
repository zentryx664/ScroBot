#include "UI/Page.hpp"
#include <algorithm>

namespace scro::ui {
using namespace style;

Page::Page(float width) : m_w(width), m_root(CCNode::create()) {}

float Page::rowHeight() const { return kSize.row; }

CCDrawNode* Page::box(float x, float top, float w, float h, ccColor4B color) {
    auto n = CCDrawNode::create();
    CCPoint q[4] = {{0.f, 0.f}, {w, 0.f}, {w, h}, {0.f, h}};
    n->drawPolygon(q, 4, f4(color), 0.f, ccc4f(0, 0, 0, 0));
    n->setPosition(x, -(top + h));
    m_root->addChild(n);
    return n;
}

CCLabelBMFont* Page::text(std::string const& s, ccColor4B color, float x, float top, float boxH, float anchorX, float line) {
    auto l = label(s, line > 0.f ? line : kSize.line, rgb(color), {anchorX, 0.5f}, {x, -(top + boxH / 2.f)});
    m_root->addChild(l);
    return l;
}

void Page::tap(float x, float top, float w, float h, std::function<void()> fn) {
    m_hits.push_back({CCRect(x, -(top + h), w, h), std::move(fn)});
}

ccColor4B Page::inkFor(Btn kind) const {
    auto const& t = theme();
    switch (kind) {
        case Btn::Accent: return t.accent;
        case Btn::Danger: return {255, 110, 110, 255};
        case Btn::Quiet: return t.muted;
        default: return t.text;
    }
}

void Page::rowLabel(std::string const& s, ccColor4B color) {
    float h = kSize.row;
    box(0.f, m_y, m_w, h, theme().card);
    auto l = text(s, color, kSize.pad + 2.f, m_y, h);
    float room = m_w - kSize.pad * 2.f - 4.f;
    float lw = l->getContentSize().width * l->getScale();
    if (lw > room) l->setScale(l->getScale() * room / lw);
    m_y += h + kSize.gap;
}

void Page::rowHeading(std::string const& s) {
    float h = kSize.row;
    box(0.f, m_y, m_w, h, theme().bg);
    text(s, theme().muted, kSize.pad + 2.f, m_y, h, 0.f, kSize.line * 0.9f);
    m_y += h + kSize.gap;
}

void Page::rowAction(std::string const& s, Btn kind, std::function<void()> fn) {
    float h = kSize.row;
    box(0.f, m_y, m_w, h, theme().card);
    auto l = text(s, inkFor(kind), kSize.pad + 2.f, m_y, h);
    float room = m_w - kSize.pad * 2.f - 4.f;
    float lw = l->getContentSize().width * l->getScale();
    if (lw > room) l->setScale(l->getScale() * room / lw);
    if (fn) tap(0.f, m_y, m_w, h, std::move(fn));
    m_y += h + kSize.gap;
}

void Page::rowToggle(std::string const& s, bool on, std::function<void()> fn) {
    auto const& t = theme();
    float h = kSize.row;
    float mark = h * 0.42f;
    box(0.f, m_y, m_w, h, t.card);
    auto l = text(s, on ? t.accent : t.text, kSize.pad + 2.f, m_y, h);
    float room = m_w - kSize.pad * 3.f - mark - 4.f;
    float lw = l->getContentSize().width * l->getScale();
    if (lw > room) l->setScale(l->getScale() * room / lw);
    box(m_w - kSize.pad - mark, m_y + (h - mark) / 2.f, mark, mark, on ? t.accent : t.off);
    tap(0.f, m_y, m_w, h, std::move(fn));
    m_y += h + kSize.gap;
}

void Page::rowStep(std::string const& s, std::string const& value, std::function<void()> dec, std::function<void()> inc) {
    auto const& t = theme();
    float h = kSize.row;
    float cell = h * 1.25f;
    box(0.f, m_y, m_w, h, t.card);
    auto l = text(s + ": ", t.text, kSize.pad + 2.f, m_y, h);
    float lw = l->getContentSize().width * l->getScale();
    auto v = text(value, t.accent, kSize.pad + 2.f + lw, m_y, h);
    float room = m_w - cell * 2.f - kSize.pad * 2.f - 4.f;
    float total = lw + v->getContentSize().width * v->getScale();
    if (total > room) {
        float k = room / total;
        l->setScale(l->getScale() * k);
        v->setScale(v->getScale() * k);
        v->setPositionX(kSize.pad + 2.f + lw * k);
    }
    float x0 = m_w - cell * 2.f - kSize.gap;
    box(x0, m_y, cell, h, t.line);
    box(x0 + cell + kSize.gap, m_y, cell, h, t.line);
    text("-", t.text, x0 + cell / 2.f, m_y, h, 0.5f);
    text("+", t.text, x0 + cell * 1.5f + kSize.gap, m_y, h, 0.5f);
    tap(x0, m_y, cell, h, std::move(dec));
    tap(x0 + cell + kSize.gap, m_y, cell, h, std::move(inc));
    m_y += h + kSize.gap;
}

void Page::rowCells(std::vector<Cell> const& cells) {
    if (cells.empty()) return;
    auto const& t = theme();
    float h = kSize.row;
    float gap = kSize.gap;
    float w = (m_w - gap * static_cast<float>(cells.size() - 1)) / static_cast<float>(cells.size());
    for (size_t i = 0; i < cells.size(); ++i) {
        float x = (w + gap) * static_cast<float>(i);
        auto const& c = cells[i];
        box(x, m_y, w, h, t.card);
        auto l = text(c.text, c.fn ? inkFor(c.kind) : t.muted, x + w / 2.f, m_y, h, 0.5f);
        float lw = l->getContentSize().width * l->getScale();
        if (lw > w - 4.f) l->setScale(l->getScale() * (w - 4.f) / lw);
        if (c.fn) tap(x, m_y, w, h, c.fn);
    }
    m_y += h + gap;
}

void Page::rowWithCells(std::string const& s, std::function<void()> fn, std::vector<Cell> const& right, float cellW) {
    auto const& t = theme();
    float h = kSize.row;
    float gap = kSize.gap;
    float rightW = cellW * static_cast<float>(right.size()) + gap * static_cast<float>(right.size());
    box(0.f, m_y, m_w, h, t.card);
    auto l = text(s, t.text, kSize.pad + 2.f, m_y, h);
    float room = m_w - rightW - kSize.pad - 2.f;
    float lw = l->getContentSize().width * l->getScale();
    if (lw > room) l->setScale(l->getScale() * room / lw);
    if (fn) tap(0.f, m_y, m_w - rightW, h, std::move(fn));
    for (size_t i = 0; i < right.size(); ++i) {
        float x = m_w - rightW + gap + (cellW + gap) * static_cast<float>(i);
        auto const& c = right[i];
        box(x, m_y, cellW, h, t.line);
        text(c.text, c.fn ? inkFor(c.kind) : t.muted, x + cellW / 2.f, m_y, h, 0.5f);
        if (c.fn) tap(x, m_y, cellW, h, c.fn);
    }
    m_y += h + gap;
}
}
