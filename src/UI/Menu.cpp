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
    std::vector<CCLayerColor*> m_side;
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

        auto bd = rect(W + 2, H + 2, ccc4(34, 34, 38, 255)); bd->setPosition(-1, -1); m_root->addChild(bd);
        m_root->addChild(rect(W, H, ccc4(14, 14, 16, static_cast<int>(250 * k))));
        m_root->addChild(rect(SB, H, ccc4(19, 19, 22, static_cast<int>(250 * k))));
        auto vd = rect(1, H, ccc4(34, 34, 38, 255)); vd->setPosition(SB, 0); m_root->addChild(vd);

        // sidebar: brand + categories + Back
        auto ac = theme::accent();
        if (auto star = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png")) {
            star->setScale(0.45f); star->setColor(ac); star->setPosition(20, H - 20); m_root->addChild(star);
        }
        m_root->addChild(label("ScroBot", 0.95f, theme::TEXT, {0.f, 0.5f}, {34, H - 20}));
        auto sm = newMenu(m_root);
        for (int i = 0; i < kNumCategories; i++) {
            auto n = CCNode::create();
            n->setContentSize({SB - 12, 24});
            auto bg = rect(SB - 12, 24, ccc4(26, 26, 30, 255));
            n->addChild(bg);
            m_side.push_back(bg);
            n->addChild(label(kCategories[i], 0.72f, theme::TEXT, {0.f, 0.5f}, {10, 12}));
            auto it = CCMenuItemSpriteExtra::create(n, this, menu_selector(ScroMenu::onTab));
            it->m_scaleMultiplier = 1.f;
            it->setTag(i); it->setPosition(SB / 2, H - 52 - i * 27.f);
            sm->addChild(it);
        }
        auto back = CCMenuItemSpriteExtra::create(button("Back", SB - 12, 24), this, menu_selector(ScroMenu::onClose));
        back->m_scaleMultiplier = 1.f; back->setPosition(SB / 2, 18);
        sm->addChild(back);

        // header
        m_title = label("", 1.0f, theme::TEXT, {0.f, 0.5f}, {SB + 14, H - 15});
        m_root->addChild(m_title);
        m_root->addChild(label(fmt::format("v{}  by  {}", Mod::get()->getVersion().toNonVString(), Mod::get()->getDeveloper()),
            0.55f, theme::MUTED, {0.f, 0.5f}, {SB + 14, H - 30}));
        m_root->addChild(label("GD 2.2081  -  Geode 5", 0.55f, theme::MUTED, {1.f, 0.5f}, {W - 12, H - 15}));
        auto hl = rect(W - SB - 1, 1, ccc4(34, 34, 38, 255)); hl->setPosition(SB + 1, H - HEAD); m_root->addChild(hl);

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
        m_root->setScale(0.96f);
        m_root->runAction(CCEaseOut::create(CCScaleTo::create(0.12f, 1.f), 2.f));
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
        for (int k = 0; k < kNumCategories; k++) {
            auto c = k == i ? theme::accentDim() : ccColor3B{26, 26, 30};
            if (animate) m_side[k]->runAction(CCTintTo::create(0.1f, c.r, c.g, c.b));
            else m_side[k]->setColor(c);
        }
        m_title->setString(kCategories[i]);
        if (m_pane) m_pane->removeFromParent();
        std::string cat = kCategories[i];
        float availH = H - HEAD - FOOT - 8.f;
        CCNode* pane; float ph;
        if (cat == "Replay") { auto mv = MacroView::create(PW, false, [this] { close(); }); pane = mv; ph = mv->height(); }
        else { auto ol = OptionList::create(cat, PW, availH, false, [this](Option* o) { setDesc(o); }); pane = ol; ph = ol->height(); }
        pane->setPosition(SB + 14, H - HEAD - 6 - ph);
        m_root->addChild(pane);
        m_pane = pane;
        setDesc(nullptr);
        if (animate) fadeTree(pane, 0.f);
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
        m_root->runAction(CCSequence::create(CCScaleTo::create(0.08f, 0.96f),
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
