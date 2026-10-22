#include "UI/FindView.hpp"
#include "UI/MacroView.hpp"
#include "UI/Menu.hpp"
#include "UI/OptionList.hpp"
#include "UI/Theme.hpp"
#include "Notifications/Notify.hpp"
#include "Utils/Draw.hpp"
#include <algorithm>
using namespace scro;
using namespace scro::draw;

namespace {
#ifdef GEODE_IS_MOBILE
// Keep the existing mobile footprint and layout.
static constexpr float W = 500.f, H = 300.f, SB = 118.f, HEAD = 40.f, FOOT = 52.f;
#else
// Desktop has more room, while retaining the same palette and chatFont.fnt.
static constexpr float W = 640.f, H = 380.f, SB = 140.f, HEAD = 40.f, FOOT = 52.f;
#endif
static constexpr float PW = W - SB - 28.f;
static constexpr char const* kGlobalTabs[] = {"General", "Speed", "Safety", "Replay", "Misc", "Find"};
static constexpr char const* kGlobalSections[] = {"General", "Speedhack", "Safety", "Recording", "Misc", "Find"};
static constexpr int kGlobalTabCount = 6;

class ScroMenu : public FLAlertLayer {
    CCNode* m_root = nullptr;
    CCNode* m_pane = nullptr;
    CCLabelBMFont* m_title = nullptr;
    CCLabelBMFont* m_desc = nullptr;
    std::vector<CCMenuItemSpriteExtra*> m_side;
    int m_tab = 0;
    int m_globalSub = 0;
    bool m_closing = false;
    float m_layoutScale = 1.f;

    CCMenu* newMenu(CCNode* parent) {
        auto m = CCMenu::create();
        m->setPosition(0, 0);
        m->setTouchPriority(-510);
        parent->addChild(m);
        return m;
    }

    CCNode* subTabNode(char const* text, bool selected, float w, float h) {
        auto n = CCNode::create();
        n->setContentSize({w, h});
        auto bg = selected ? theme::accentDim() : ccColor3B{24, 24, 29};
        n->addChild(rect(w, h, ccc4(bg.r, bg.g, bg.b, 255)));
        auto l = label(text, 0.58f, selected ? theme::TEXT : theme::MUTED, {0.5f, 0.5f}, {w / 2, h / 2});
        l->limitLabelWidth(w - 4.f, 0.58f, 0.35f);
        n->addChild(l);
        return n;
    }

    void buildGlobalPane(bool animate) {
        float bodyH = H - HEAD - FOOT - 8.f;
        auto pane = CCNode::create();
        pane->setContentSize({PW, bodyH});

        auto tabs = newMenu(pane);
        float tabH = 20.f, gap = 3.f;
        float tabW = (PW - gap * (kGlobalTabCount - 1)) / kGlobalTabCount;
        float tabY = bodyH - tabH / 2;
        for (int i = 0; i < kGlobalTabCount; ++i) {
            auto it = CCMenuItemSpriteExtra::create(subTabNode(kGlobalTabs[i], m_globalSub == i, tabW, tabH),
                this, menu_selector(ScroMenu::onGlobalSub));
            it->m_scaleMultiplier = 1.f;
            it->setTag(i);
            it->setPosition(tabW / 2 + i * (tabW + gap), tabY);
            tabs->addChild(it);
        }

        float innerH = bodyH - tabH - 6.f;
        CCNode* content = nullptr;
        if (m_globalSub == 5) {
            content = FindView::create(PW, innerH, [this](Option* o) { onFindSelect(o); });
        } else {
            content = OptionList::create("Global", PW, innerH, animate,
                [this](Option* o) { setDesc(o); }, kGlobalSections[m_globalSub]);
        }
        if (content) {
            float contentY = m_globalSub == 5 ? 0.f : std::max(0.f, innerH - content->getContentSize().height);
            content->setPosition(0, contentY);
            pane->addChild(content);
        }
        pane->setPosition(SB + 14.f, H - HEAD - 6.f - bodyH);
        m_root->addChild(pane);
        m_pane = pane;
        if (animate) {
            pane->setPositionX(pane->getPositionX() + 10.f);
            fadeTree(pane, 0.f);
            pane->runAction(CCEaseOut::create(CCMoveBy::create(0.16f, {-10.f, 0.f}), 2.f));
        }
    }

    bool init() {
        if (!FLAlertLayer::init(120)) return false;
        m_noElasticity = true;
        m_mainLayer = CCLayer::create();
        this->addChild(m_mainLayer);
        this->setKeypadEnabled(true);
        this->setTouchEnabled(true);

        auto win = CCDirector::get()->getWinSize();
        float sx = (win.width - 16.f) / W;
        float sy = (win.height - 16.f) / H;
        m_layoutScale = std::clamp(std::min({1.f, sx, sy}), 0.55f, 1.f);
        float k = find("menuopacity").f / 100.f;
        m_root = CCNode::create();
        m_root->setContentSize({W, H});
        m_root->setAnchorPoint({0.5f, 0.5f});
        m_root->setPosition(win.width / 2, win.height / 2);
        m_mainLayer->addChild(m_root);

        m_root->addChild(rect(W, H, ccc4(16, 16, 18, static_cast<int>(248 * k))));
        m_root->addChild(rect(SB, H, ccc4(12, 12, 14, static_cast<int>(248 * k))));

        // Existing Scro sidebar, same font and monochrome styling.
        m_root->addChild(label("Scro", 0.92f, theme::TEXT, {0.f, 0.5f}, {16, H - 20}));
        auto sm = newMenu(m_root);
        float sideTop = H - 52.f, sideBottom = 43.f;
        float sideStep = (sideTop - sideBottom) / std::max(1, kNumCategories - 1);
        for (int i = 0; i < kNumCategories; i++) {
            auto n = CCNode::create();
            n->setContentSize({SB - 12, 24});
            auto mark = rect(2, 14, ccc4(0, 0, 0, 0)); mark->setID("mark"); mark->setPosition(0, 5); n->addChild(mark);
            auto lbl = label(kCategories[i], 0.68f, theme::MUTED, {0.f, 0.5f}, {10, 12}); lbl->setID("lbl"); n->addChild(lbl);
            auto it = CCMenuItemSpriteExtra::create(n, this, menu_selector(ScroMenu::onTab));
            it->m_scaleMultiplier = 1.f;
            it->setTag(i); it->setPosition(SB / 2, sideTop - i * sideStep);
            sm->addChild(it);
            m_side.push_back(it);
        }
        auto back = CCMenuItemSpriteExtra::create(button("Back", SB - 12, 22), this, menu_selector(ScroMenu::onClose));
        back->m_scaleMultiplier = 1.f; back->setPosition(SB / 2, 16);
        sm->addChild(back);

        m_title = label("", 0.95f, theme::TEXT, {0.f, 0.5f}, {SB + 14, H - 16});
        m_root->addChild(m_title);
        auto hl = rect(W - SB, 1, ccc4(28, 28, 32, 255)); hl->setPosition(SB, H - HEAD); m_root->addChild(hl);

        auto fl = rect(W - SB - 1, 1, ccc4(34, 34, 38, 255)); fl->setPosition(SB + 1, FOOT); m_root->addChild(fl);
        m_desc = label("", 0.62f, theme::MUTED, {0.f, 0.5f}, {SB + 14, FOOT - 12});
        m_root->addChild(m_desc);
        auto fm = newMenu(m_root);
        auto sv = CCMenuItemSpriteExtra::create(button("Save", 62, 24), this, menu_selector(ScroMenu::onSave));
        sv->m_scaleMultiplier = 1.f; sv->setPosition(W - 12 - 31, 17);
        auto rs = CCMenuItemSpriteExtra::create(button("Reset", 62, 24), this, menu_selector(ScroMenu::onReset));
        rs->m_scaleMultiplier = 1.f; rs->setPosition(W - 12 - 62 - 6 - 31, 17);
        fm->addChild(sv); fm->addChild(rs);

        showTab(0, false);
        m_root->setScale(m_layoutScale * 0.95f);
        m_root->setPositionY(m_root->getPositionY() - 8.f * m_layoutScale);
        m_root->runAction(CCSpawn::create(
            CCEaseExponentialOut::create(CCScaleTo::create(0.2f, m_layoutScale)),
            CCEaseExponentialOut::create(CCMoveBy::create(0.2f, {0.f, 8.f * m_layoutScale})), nullptr));
        fadeTree(m_root, 0.f);
        return true;
    }

    void setDesc(Option* o) {
        std::string t = o ? (o->desc.empty() ? o->name : o->desc) : "Tap an option to see what it does.";
        m_desc->setString(t.c_str());
        // Reserve room for Reset/Save so help text never runs underneath the buttons.
        float helpW = std::max(60.f, W - SB - 154.f);
        m_desc->limitLabelWidth(helpW, 0.62f, 0.3f);
    }

    void showTab(int i, bool animate) {
        m_tab = i;
        auto ac = theme::accent();
        for (int k = 0; k < kNumCategories; k++) {
            auto n = m_side[k]->getNormalImage();
            auto mark = n->getChildByID("mark");
            auto lbl = typeinfo_cast<CCLabelBMFont*>(n->getChildByID("lbl"));
            ccColor3B mc = k == i ? ac : ccColor3B{0, 0, 0};
            GLubyte ma = k == i ? 255 : 0;
            ccColor3B lc = k == i ? theme::TEXT : theme::MUTED;
            if (animate) {
                mark->stopAllActions(); lbl->stopAllActions();
                mark->runAction(CCTintTo::create(0.1f, mc.r, mc.g, mc.b));
                mark->runAction(CCFadeTo::create(0.1f, ma));
                lbl->runAction(CCTintTo::create(0.1f, lc.r, lc.g, lc.b));
            } else {
                auto markColor = static_cast<CCLayerColor*>(mark);
                markColor->setColor(mc);
                markColor->setOpacity(ma);
                lbl->setColor(lc);
            }
        }
        m_title->setString(kCategories[i]);
        if (m_pane) m_pane->removeFromParent();
        std::string cat = kCategories[i];
        if (cat == "Global") {
            buildGlobalPane(animate);
            setDesc(nullptr);
            return;
        }

        float availH = H - HEAD - FOOT - 8.f;
        CCNode* pane = nullptr;
        float ph = availH;
#ifdef GEODE_IS_MOBILE
        constexpr bool isDesktop = false;
#else
        constexpr bool isDesktop = true;
#endif
        if (cat == "Replay") {
            auto mv = MacroView::create(PW, isDesktop, [this] { close(); });
            pane = mv; ph = mv->height();
        } else {
            auto ol = OptionList::create(cat, PW, availH, animate, [this](Option* o) { setDesc(o); });
            pane = ol; ph = ol->height();
        }
        pane->setPosition(SB + 14.f, H - HEAD - 6.f - ph);
        m_root->addChild(pane);
        m_pane = pane;
        setDesc(nullptr);
        if (animate) {
            pane->setPositionX(pane->getPositionX() + 10.f);
            fadeTree(pane, 0.f);
            pane->runAction(CCEaseOut::create(CCMoveBy::create(0.16f, {-10.f, 0.f}), 2.f));
        }
    }

    void onFindSelect(Option* o) {
        if (!o) return;
        int target = -1;
        for (int i = 0; i < kNumCategories; ++i) {
            if (o->cat == kCategories[i]) { target = i; break; }
        }
        if (target < 0) return;
        if (o->cat == "Global") {
            m_globalSub = 0;
            for (int i = 0; i < kGlobalTabCount - 1; ++i) {
                if (o->section == kGlobalSections[i]) { m_globalSub = i; break; }
            }
        }
        showTab(target, true);
        setDesc(o);
    }

    void onTab(CCObject* s) {
        int i = static_cast<CCNode*>(s)->getTag();
        if (i != m_tab && !m_closing) showTab(i, true);
    }
    void onGlobalSub(CCObject* s) {
        int i = static_cast<CCNode*>(s)->getTag();
        if (i >= 0 && i < kGlobalTabCount && i != m_globalSub && !m_closing) {
            m_globalSub = i;
            showTab(m_tab, true);
        }
    }
    void onReset(CCObject*) {
        if (std::string(kCategories[m_tab]) == "Replay") return;
        reset(kCategories[m_tab]);
        changed();
        showTab(m_tab, false);
        notify("Category reset");
    }
    void onSave(CCObject*) { save(); notify("Settings saved"); }
    void onClose(CCObject*) { close(); }
    void finish() { this->setKeypadEnabled(false); this->setTouchEnabled(false); this->removeFromParentAndCleanup(true); }
    void keyBackClicked() override { close(); }

public:
    void close() {
        if (m_closing) return;
        m_closing = true;
        m_root->runAction(CCSpawn::create(
            CCEaseIn::create(CCScaleTo::create(0.14f, m_layoutScale * 0.95f), 2.f),
            CCEaseIn::create(CCMoveBy::create(0.14f, {0.f, -8.f * m_layoutScale}), 2.f), nullptr));
        m_root->runAction(CCSequence::create(CCDelayTime::create(0.1f),
            CCCallFunc::create(this, callfunc_selector(ScroMenu::finish)), nullptr));
    }
    static ScroMenu* create() {
        auto r = new ScroMenu();
        if (r->init()) { r->autorelease(); return r; }
        delete r;
        return nullptr;
    }
};
}

void scro::ui::open() {
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene || scene->getChildByID("scrobot-menu")) return;
    if (auto m = ScroMenu::create()) { m->setID("scrobot-menu"); m->show(); }
}
void scro::ui::toggle() {
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene) return;
    if (auto m = scene->getChildByID("scrobot-menu")) static_cast<ScroMenu*>(m)->close();
    else open();
}
