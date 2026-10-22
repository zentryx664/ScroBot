#include "UI/MacroView.hpp"
#include "UI/Menu.hpp"
#include "UI/OptionList.hpp"
#include "UI/Theme.hpp"
#include "Notifications/Notify.hpp"
#include "Utils/Draw.hpp"
using namespace scro;
using namespace scro::draw;

namespace {
// One screen for PC and mobile: category list on the left, settings on the right,
// description line and Reset / Save at the bottom.
class ScroMenu : public FLAlertLayer {
    static constexpr float W = 500.f, H = 300.f, SB = 118.f, HEAD = 40.f, FOOT = 52.f;
    static constexpr float PW = W - SB - 28.f;           // settings pane width
    CCNode* m_root = nullptr;
    CCNode* m_pane = nullptr;
    CCLabelBMFont* m_title = nullptr;
    CCLabelBMFont* m_desc = nullptr;
    std::vector<CCMenuItemSpriteExtra*> m_side;
    int m_tab = 0;
    bool m_closing = false;

    CCMenu* newMenu(CCNode* parent) {
        auto m = CCMenu::create();
        m->setPosition(0, 0);
        m->setTouchPriority(-510);
        parent->addChild(m);
        return m;
    }

    bool init() {
        if (!FLAlertLayer::init(120)) return false;
        m_noElasticity = true;
        m_mainLayer = CCLayer::create();
        this->addChild(m_mainLayer);
        this->setKeypadEnabled(true);
        this->setTouchEnabled(true);

        auto win = CCDirector::get()->getWinSize();
        float k = find("menuopacity").f / 100.f;
        m_root = CCNode::create();
        m_root->setContentSize({W, H});
        m_root->setAnchorPoint({0.5f, 0.5f});
        m_root->setPosition(win.width / 2, win.height / 2);
        m_mainLayer->addChild(m_root);

        m_root->addChild(rect(W, H, ccc4(16, 16, 18, static_cast<int>(248 * k))));
        m_root->addChild(rect(SB, H, ccc4(12, 12, 14, static_cast<int>(248 * k))));

        // sidebar: plain wordmark + categories + Back, no icon
        m_root->addChild(label("Scro", 0.92f, theme::TEXT, {0.f, 0.5f}, {16, H - 20}));
        auto sm = newMenu(m_root);
        for (int i = 0; i < kNumCategories; i++) {
            auto n = CCNode::create();
            n->setContentSize({SB - 12, 24});
            auto mark = rect(2, 14, ccc4(0, 0, 0, 0)); mark->setID("mark"); mark->setPosition(0, 5); n->addChild(mark);
            auto lbl = label(kCategories[i], 0.68f, theme::MUTED, {0.f, 0.5f}, {10, 12}); lbl->setID("lbl"); n->addChild(lbl);
            auto it = CCMenuItemSpriteExtra::create(n, this, menu_selector(ScroMenu::onTab));
            it->m_scaleMultiplier = 1.f;
            it->setTag(i); it->setPosition(SB / 2, H - 52 - i * 26.f);
            sm->addChild(it);
            m_side.push_back(it);
        }
        auto back = CCMenuItemSpriteExtra::create(button("Back", SB - 12, 22), this, menu_selector(ScroMenu::onClose));
        back->m_scaleMultiplier = 1.f; back->setPosition(SB / 2, 16);
        sm->addChild(back);

        // header: just the category name, no version clutter
        m_title = label("", 0.95f, theme::TEXT, {0.f, 0.5f}, {SB + 14, H - 16});
        m_root->addChild(m_title);
        auto hl = rect(W - SB, 1, ccc4(28, 28, 32, 255)); hl->setPosition(SB, H - HEAD); m_root->addChild(hl);

        // footer: description + Reset / Save
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
        m_root->setScale(0.95f);
        m_root->setPositionY(m_root->getPositionY() - 8.f);
        m_root->runAction(CCSpawn::create(
            CCEaseExponentialOut::create(CCScaleTo::create(0.2f, 1.f)),
            CCEaseExponentialOut::create(CCMoveBy::create(0.2f, {0.f, 8.f})), nullptr));
        fadeTree(m_root, 0.f);
        return true;
    }

    void setDesc(Option* o) {
        std::string t = o ? (o->desc.empty() ? o->name : o->desc) : "Tap an option to see what it does.";
        m_desc->setString(t.c_str());
        m_desc->limitLabelWidth(PW, 0.62f, 0.3f);
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
                static_cast<CCLayerColor*>(mark)->setColor(mc);
                mark->setOpacity(ma);
                lbl->setColor(lc);
            }
        }
        m_title->setString(kCategories[i]);
        if (m_pane) m_pane->removeFromParent();
        std::string cat = kCategories[i];
        float availH = H - HEAD - FOOT - 8.f;
        CCNode* pane; float ph;
        if (cat == "Replay") { auto mv = MacroView::create(PW, false, [this] { close(); }); pane = mv; ph = mv->height(); }
        else { auto ol = OptionList::create(cat, PW, availH, animate, [this](Option* o) { setDesc(o); }); pane = ol; ph = ol->height(); }
        pane->setPosition(SB + 14, H - HEAD - 6 - ph);
        m_root->addChild(pane);
        m_pane = pane;
        setDesc(nullptr);
        if (animate) {
            // soft slide + fade when switching categories, not just an instant swap
            pane->setPositionX(pane->getPositionX() + 10.f);
            fadeTree(pane, 0.f);
            pane->runAction(CCEaseOut::create(CCMoveBy::create(0.16f, {-10.f, 0.f}), 2.f));
        }
    }

    void onTab(CCObject* s) {
        int i = static_cast<CCNode*>(s)->getTag();
        if (i != m_tab && !m_closing) showTab(i, true);
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
            CCEaseIn::create(CCScaleTo::create(0.14f, 0.95f), 2.f),
            CCEaseIn::create(CCMoveBy::create(0.14f, {0.f, -8.f}), 2.f), nullptr));
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
