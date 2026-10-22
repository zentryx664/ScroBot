#include "UI/OptionList.hpp"
#include "UI/Theme.hpp"
#include "Utils/Draw.hpp"
#include <algorithm>
using namespace scro;
using namespace scro::draw;

OptionList* OptionList::create(std::string const& cat, float w, float availH, bool animate, std::function<void(Option*)> onSel, std::string const& section) {
    auto r = new OptionList();
    if (r->init() && r->setup(cat, w, availH, animate, std::move(onSel), section)) { r->autorelease(); return r; }
    delete r;
    return nullptr;
}

bool OptionList::setup(std::string const& cat, float w, float availH, bool animate, std::function<void(Option*)> onSel, std::string const& section) {
    m_w = w; m_onSel = std::move(onSel);
    m_opts = inCategory(cat);
    if (!section.empty()) {
        m_opts.erase(std::remove_if(m_opts.begin(), m_opts.end(), [&](Option* o) { return o->section != section; }), m_opts.end());
    }
    int sections = 0;
    if (section.empty()) {
        std::string last;
        for (auto o : m_opts) if (o->section != last) { sections++; last = o->section; }
    }
    float secH = 20.f;
    float r = std::clamp((availH - sections * secH) / std::max<size_t>(m_opts.size(), 1), 22.f, 28.f);
    m_h = m_opts.size() * r + sections * secH;
    this->setContentSize({w, m_h});
    m_vals.assign(m_opts.size(), nullptr);

    auto menu = CCMenu::create();
    menu->setPosition(0, 0);
    menu->setTouchPriority(-510);
    this->addChild(menu);

    auto ac = theme::accent();
    float y = m_h, pillW = 46.f, bs = r - 8.f, vw = 56.f, gap = 3.f;
    std::string last;
    for (size_t i = 0; i < m_opts.size(); i++) {
        auto o = m_opts[i];
        float delay = animate ? 0.02f * i : 0.f;
        int idx = static_cast<int>(i);
        if (section.empty() && o->section != last) {               // section heading
            last = o->section;
            y -= secH;
            this->addChild(label(o->section, 0.62f, theme::MUTED, {0.f, 0.5f}, {2.f, y + secH / 2 + 1}));
            auto ln = rect(w, 1, ccc4(40, 40, 46, 255)); ln->setPosition(0, y + 1); this->addChild(ln);
        }
        y -= r;
        float cy = y + r / 2;
        auto sep = rect(w, 1, ccc4(24, 24, 28, 255)); sep->setPosition(0, y); this->addChild(sep);

        if (o->kind == Kind::Toggle) {                              // whole row taps; ON/OFF pill on the right
            auto n = CCNode::create();
            n->setContentSize({w, r});
            auto nm = label(o->name, 0.74f, theme::TEXT, {0.f, 0.5f}, {4.f, r / 2});
            nm->limitLabelWidth(w - pillW - 16, 0.74f, 0.3f);
            n->addChild(nm);
            auto pill = rect(pillW, r - 8, o->b ? ccc4(ac.r, ac.g, ac.b, 255) : ccc4(34, 34, 38, 255));
            pill->setID("pill"); pill->setPosition(w - pillW, 4.f);
            n->addChild(pill);
            auto pt = label(o->b ? "ON" : "OFF", 0.66f, o->b ? theme::onAccent() : theme::MUTED, {0.5f, 0.5f}, {w - pillW / 2, r / 2});
            pt->setID("pt");
            n->addChild(pt);
            auto it = CCMenuItemSpriteExtra::create(n, this, menu_selector(OptionList::onToggle));
            it->m_scaleMultiplier = 1.f;
            it->setTag(idx); it->setPosition(w / 2, cy);
            menu->addChild(it);
            if (animate) fadeTree(it, delay);
        } else if (o->kind == Kind::Step) {
            auto nm = label(o->name, 0.74f, theme::TEXT, {0.f, 0.5f}, {4.f, cy});
            nm->limitLabelWidth(w - (bs * 2 + vw + gap * 2) - 16, 0.74f, 0.3f);
            this->addChild(nm);
            auto box = rect(vw, bs, ccc4(24, 24, 28, 255)); box->setPosition(w - bs - gap - vw, cy - bs / 2); this->addChild(box);
            auto mi = CCMenuItemSpriteExtra::create(button("-", bs, bs), this, menu_selector(OptionList::onStep));
            mi->setTag(idx * 2); mi->setPosition(w - bs - gap - vw - gap - bs / 2, cy);
            auto pi = CCMenuItemSpriteExtra::create(button("+", bs, bs), this, menu_selector(OptionList::onStep));
            pi->setTag(idx * 2 + 1); pi->setPosition(w - bs / 2, cy);
            menu->addChild(mi); menu->addChild(pi);
            auto v = label(o->text(), 0.7f, ac, {0.5f, 0.5f}, {w - bs - gap - vw / 2, cy});
            v->limitLabelWidth(vw - 4, 0.7f, 0.3f);
            this->addChild(v);
            m_vals[i] = v;
            if (animate) { fadeTree(nm, delay); fadeTree(box, delay); fadeTree(mi, delay); fadeTree(pi, delay); fadeTree(v, delay); }
        } else {                                                    // action: name + Open button
            auto nm = label(o->name, 0.74f, theme::TEXT, {0.f, 0.5f}, {4.f, cy});
            this->addChild(nm);
            auto it = CCMenuItemSpriteExtra::create(button("Open", pillW, r - 8), this, menu_selector(OptionList::onAction));
            it->m_scaleMultiplier = 1.f;
            it->setTag(idx); it->setPosition(w - pillW / 2, cy);
            menu->addChild(it);
            if (animate) { fadeTree(nm, delay); fadeTree(it, delay); }
        }
    }
    return true;
}

void OptionList::onToggle(CCObject* s) {
    auto it = static_cast<CCMenuItemSpriteExtra*>(s);
    auto o = m_opts[it->getTag()];
    o->b = !o->b;
    auto n = it->getNormalImage();
    auto c = o->b ? theme::accent() : ccColor3B{34, 34, 38};
    if (auto pill = n->getChildByID("pill")) pill->runAction(CCTintTo::create(0.1f, c.r, c.g, c.b));
    if (auto pt = typeinfo_cast<CCLabelBMFont*>(n->getChildByID("pt"))) {
        pt->setString(o->b ? "ON" : "OFF");
        pt->setColor(o->b ? theme::onAccent() : theme::MUTED);
    }
    changed();
    if (o->onChange) o->onChange();
    if (m_onSel) m_onSel(o);
}
void OptionList::onStep(CCObject* s) {
    int tag = static_cast<CCNode*>(s)->getTag(), i = tag / 2;
    auto o = m_opts[i];
    float v = std::clamp(o->f + ((tag & 1) ? o->st : -o->st), o->mn, o->mx);
    o->f = std::round(v / o->st) * o->st;
    if (auto l = m_vals[i]) { l->setString(o->text().c_str()); l->limitLabelWidth(52.f, 0.7f, 0.3f); }
    changed();
    if (o->onChange) o->onChange();
    if (m_onSel) m_onSel(o);
}
void OptionList::onAction(CCObject* s) {
    auto o = m_opts[static_cast<CCNode*>(s)->getTag()];
    if (o->action) o->action();
    if (m_onSel) m_onSel(o);
}
