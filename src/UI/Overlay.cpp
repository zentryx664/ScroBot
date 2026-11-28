#include "UI/Overlay.hpp"
#include "Hacks/FpsBoost.hpp"
#include "Hacks/FrameStep.hpp"
#include "Macro/Bot.hpp"
#include "UI/Pages.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/AppDelegate.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/OverlayManager.hpp>
#include <algorithm>
#include <cmath>

namespace scro::ui {
using namespace style;

namespace {
Overlay* g_overlay = nullptr;

scro::Option& menuSize = scro::step("Theme", "uiscale", "Menu Size", "Scales every window.", 100.f, 60.f, 250.f, 10.f, "%", 10);
scro::Option& ballSize = scro::step("Theme", "ballsize", "Ball Size", "Size of the floating ball.", 100.f, 70.f, 150.f, 10.f, "%", 20);
scro::Option& tabResize = scro::step("Theme", "tabresize", "Resize", "Resizes the tabs, rows and buttons.", 100.f, 60.f, 180.f, 10.f, "%", 12);
scro::Option& uiGradient = scro::step("Theme", "uigradient", "Gradient", "Gradient colours for the window title bars.", 0.f, 0.f, 6.f, 1.f, "", 15);
scro::Option& ballReset = scro::action("Theme", "ballreset", "Reset Ball",
    "Moves the ball back to where it starts.", [] { if (g_overlay) g_overlay->resetBallPosition(); }, 30);
scro::Option& layoutReset = scro::action("Theme", "layoutreset", "Reset Layout",
    "Puts every window back in its starting place.", [] { if (g_overlay) g_overlay->resetLayout(); }, 40);
bool const wired = ([] {
    menuSize.onChange = [] { if (g_overlay) g_overlay->applyScale(); };
    ballSize.onChange = [] { if (g_overlay) g_overlay->applyBall(); };
    tabResize.onChange = [] { if (g_overlay) g_overlay->applyTabSize(); };
    uiGradient.names = {"Off", "Aurora", "Sunset", "Ocean", "Forest", "Candy", "Fire"};
    uiGradient.onChange = [] { if (g_overlay) g_overlay->rebuildAll(); };
    return true;
})();

constexpr float kDragSlop = 6.f;
constexpr float kMargin = 4.f;
constexpr float kSavedMissing = -1000.f;
}

// ---- clipping ------------------------------------------------------------------------------

ClipNode* ClipNode::create(CCSize size) {
    auto n = new ClipNode();
    if (n->init()) {
        n->setContentSize(size);
        n->autorelease();
        return n;
    }
    delete n;
    return nullptr;
}

void ClipNode::visit() {
    auto a = this->convertToWorldSpace({0.f, 0.f});
    auto b = this->convertToWorldSpace({this->getContentSize().width, this->getContentSize().height});
    glEnable(GL_SCISSOR_TEST);
    CCDirector::get()->getOpenGLView()->setScissorInPoints(a.x, a.y, b.x - a.x, b.y - a.y);
    CCNode::visit();
    glDisable(GL_SCISSOR_TEST);
}

// ---- creation ------------------------------------------------------------------------------

Overlay* Overlay::get() { return g_overlay; }

void Overlay::install() {
    if (g_overlay) return;

    // Attach to Geode's persistent overlay scene graph. This gives CCLayer its
    // normal onEnter/onExit lifecycle so its touch delegate is registered. 
    auto manager = geode::OverlayManager::get();
    if (!manager) {
        log::error("ScroBot: OverlayManager is unavailable; floating menu was not installed");
        return;
    }

    auto o = new Overlay();
    if (o->init() && o->setup()) {
        manager->addChild(o, 1000);
        o->autorelease();
        g_overlay = o;
    } else {
        delete o;
    }
}

bool Overlay::setup() {
    this->setTouchMode(kCCTouchesOneByOne);
    this->setTouchPriority(-1000);
    this->setTouchEnabled(true);
    this->setKeyboardEnabled(true);
#ifdef GEODE_IS_DESKTOP
    this->setMouseEnabled(true);
#endif
    this->scheduleUpdate();

    auto mod = Mod::get();
    m_ballFrac = ccp(mod->getSavedValue<float>("ui-ball-x", kMobile ? 0.05f : 0.04f), mod->getSavedValue<float>("ui-ball-y", 0.6f));
    m_win = CCDirector::get()->getWinSize();

    m_menuAnim = CCNode::create();
    m_menuAnim->setVisible(false);
    this->addChild(m_menuAnim, 5);
    m_menu = CCNode::create();
    m_menuAnim->addChild(m_menu);

    m_info = CCNode::create();
    this->addChild(m_info, 6);

    m_ball = ballButton(kSize.ball);
    m_ball->setAnchorPoint({0.5f, 0.5f});
    this->addChild(m_ball, 10);

    makeWindows();
    placeBall();
    return true;
}

// ---- geometry ------------------------------------------------------------------------------

float Overlay::menuScale() const { return std::max(0.3f, menuSize.f / 100.f); }
float Overlay::ballRadius() const { return kSize.ball * ballSize.f / 100.f; }
float Overlay::menuW() const { return m_win.width / menuScale(); }
float Overlay::menuH() const { return m_win.height / menuScale(); }
float Overlay::maxBodyH() const { return std::max(40.f, menuH() - titleH() - kMargin * 2.f - (kMobile ? 0.f : 14.f)); }

CCPoint Overlay::toMenu(CCPoint world) const { return m_menu->convertToNodeSpace(world); }

void Overlay::clampBall() {
    float r = ballRadius() + 2.f;
    m_ballPos.x = std::clamp(m_ballPos.x, r, std::max(r, m_win.width - r));
    m_ballPos.y = std::clamp(m_ballPos.y, r, std::max(r, m_win.height - r));
}

void Overlay::setBallVisualScale(float multiplier) {
    if (!m_ball) return;
    auto const size = m_ball->getContentSize();
    if (size.width <= 0.f || size.height <= 0.f) return;

    float const diameter = kSize.ball * 2.f * multiplier;
    m_ball->setScaleX(diameter / size.width);
    m_ball->setScaleY(diameter / size.height);
}

void Overlay::placeBall() {
    m_ballPos = ccp(m_ballFrac.x * m_win.width, m_ballFrac.y * m_win.height);
    clampBall();
    setBallVisualScale(ballSize.f / 100.f);
    m_ball->setPosition(m_ballPos);
}

void Overlay::clampWindow(Win& w) {
    float H = titleH() + (w.collapsed ? 0.f : w.bodyH);
    w.x = std::clamp(w.x, 0.f, std::max(0.f, menuW() - w.width));
    w.y = std::clamp(w.y, std::min(H, menuH()), std::max(H, menuH()));
}

void Overlay::applyScale() {
    m_menu->setScale(menuScale());
    for (auto& w : m_wins) clampWindow(w);
    if (m_open) drawAll();
    savePositions();
}

void Overlay::applyTabSize() {
    for (auto& w : m_wins) {
        w.width = kSize.width * w.widthFactor * tabScale();
        clampWindow(w);
    }
    if (m_open) drawAll();
    savePositions();
}

void Overlay::drawOne(Win& w) {
    drawWindow(w);
    for (size_t k = 0; k < m_order.size(); ++k)
        if (&m_wins[m_order[k]] == &w) w.node->setZOrder(static_cast<int>(k));
}

void Overlay::redrawFor(std::string const& id) {
    if (!m_open) return;
    // Macro / Editor windows depend on each other, everything else only on itself.
    if (id != "Macro" && id != "Editor" && id != "Theme") {
        for (auto& w : m_wins) if (w.id == id) { drawOne(w); return; }
    }
    drawAll();
}

void Overlay::pulseBall() {
    if (!m_ball) return;
    auto const size = m_ball->getContentSize();
    if (size.width <= 0.f) return;
    float base = kSize.ball * 2.f * (ballSize.f / 100.f) / size.width;
    m_ball->stopAllActions();
    m_ball->runAction(CCSequence::create(
        CCScaleTo::create(0.07f, base * 0.84f),
        CCEaseBackOut::create(CCScaleTo::create(0.24f, base)), nullptr));
}

void Overlay::applyBall() {
    placeBall();
    savePositions();
}

void Overlay::resetBallPosition() {
    m_ballFrac = ccp(kMobile ? 0.05f : 0.04f, 0.6f);
    placeBall();
    savePositions();
}

// ---- windows -------------------------------------------------------------------------------

void Overlay::makeWindows() {
    auto add = [this](std::string id, float factor, std::function<void(Page&)> build) {
        Win w;
        w.id = std::move(id);
        w.widthFactor = factor;
        w.build = std::move(build);
        m_wins.push_back(std::move(w));
    };
    for (int i = 0; i < scro::kNumCategories; ++i) {
        std::string cat = scro::kCategories[i];
        add(cat, 1.f, [this, cat](Page& p) { buildOptions(p, *this, cat); });
    }
    add("Macro", 1.35f, [this](Page& p) { buildMacro(p, *this); });
    add("Editor", kMobile ? 2.2f : 2.6f, [this](Page& p) { buildEditor(p, *this); });
    add("Theme", 1.2f, [this](Page& p) { buildTheme(p, *this); });

    auto mod = Mod::get();
    for (size_t i = 0; i < m_wins.size(); ++i) {
        auto& w = m_wins[i];
        w.width = kSize.width * w.widthFactor * tabScale();
        auto key = "w-" + w.id;
        float fx = mod->getSavedValue<float>(key + "-x", kSavedMissing);
        float fy = mod->getSavedValue<float>(key + "-y", kSavedMissing);
        w.collapsed = mod->getSavedValue<bool>(key + "-c", false);
        if (fx > kSavedMissing && fy > kSavedMissing) {
            w.fx = fx;
            w.fy = fy;
            w.x = fx * m_win.width / menuScale();
            w.y = fy * m_win.height / menuScale();
            w.placed = true;
        }
        m_order.push_back(i);
    }
    m_menu->setScale(menuScale());
}

void Overlay::measure(Win& w) {
    Page page(w.width);
    w.build(page);
    w.contentH = std::max(0.f, page.height() - kSize.gap);
    w.bodyH = std::min(w.contentH, maxBodyH());
}

// First start: windows fill the screen from the left, each in the shortest column.
// A window that would run off the bottom starts collapsed.
void Overlay::autoLayout() {
    float const gap = 3.f;
    float const unit = kSize.width * tabScale() + gap;
    int const cols = std::max(1, static_cast<int>((menuW() - kMargin) / unit));
    std::vector<float> bottom(static_cast<size_t>(cols), kMargin);
    for (auto& w : m_wins) {
        if (w.placed) continue;
        measure(w);
        int span = std::min(cols, std::max(1, static_cast<int>(std::ceil((w.width + gap) / unit))));
        int best = 0;
        float bestY = 1e9f;
        for (int c = 0; c + span <= cols; ++c) {
            float y = 0.f;
            for (int k = 0; k < span; ++k) y = std::max(y, bottom[static_cast<size_t>(c + k)]);
            if (y < bestY) { bestY = y; best = c; }
        }
        float full = titleH() + w.bodyH;
        if (bestY + full > menuH() - kMargin) w.collapsed = true;
        float h = titleH() + (w.collapsed ? 0.f : w.bodyH);
        w.x = kMargin + static_cast<float>(best) * unit;
        w.y = menuH() - bestY;
        w.placed = true;
        w.fx = w.x * menuScale() / m_win.width;
        w.fy = w.y * menuScale() / m_win.height;
        for (int k = 0; k < span; ++k) bottom[static_cast<size_t>(best + k)] = bestY + h + gap;
    }
}

void Overlay::drawWindow(Win& w) {
    auto const& t = theme();
    if (w.node) w.node->removeFromParent();
    w.node = nullptr;
    w.inner = nullptr;
    w.bar = nullptr;
    w.hits.clear();

    Page page(w.width);
    w.build(page);
    w.contentH = std::max(0.f, page.height() - kSize.gap);
    w.bodyH = std::min(w.contentH, maxBodyH());
    w.scroll = std::clamp(w.scroll, 0.f, std::max(0.f, w.contentH - w.bodyH));
    clampWindow(w);

    float const W = w.width, T = titleH();
    float const body = w.collapsed ? 0.f : w.bodyH;
    float const H = T + body;

    auto node = CCNode::create();
    node->setPosition(w.x, w.y - H);

    if (!w.collapsed) {
        auto bg = CCDrawNode::create();
        CCPoint q[4] = {{0.f, 0.f}, {W, 0.f}, {W, body}, {0.f, body}};
        bg->drawPolygon(q, 4, f4(t.bg), 0.f, ccc4f(0, 0, 0, 0));
        node->addChild(bg);

        auto clip = ClipNode::create({W, body});
        node->addChild(clip);
        clip->addChild(page.node());
        w.inner = page.node();
        w.inner->setPosition(0.f, body + w.scroll);
        w.hits = std::move(page.hits());
        cullWindow(w);

        w.bar = CCDrawNode::create();
        node->addChild(w.bar, 3);
        updateBar(w);
    }

    // title bar
    bool const grad = gradientIndex() > 0;
    ccColor4B const ink = grad ? ccColor4B{255, 255, 255, 255} : t.onAccent;
    CCDrawNode* title;
    if (grad) {
        title = gradientBar(W, T);
        title->setPosition(0.f, body);
    } else {
        title = CCDrawNode::create();
        CCPoint tq[4] = {{0.f, body}, {W, body}, {W, H}, {0.f, H}};
        title->drawPolygon(tq, 4, f4(t.accent), 0.f, ccc4f(0, 0, 0, 0));
    }
    node->addChild(title, 4);
    node->addChild(label(w.id, T * 0.62f, rgb(ink), {0.5f, 0.5f}, {W / 2.f, body + T / 2.f}), 5);
    // collapse glyph: a bar, plus a second bar when collapsed
    auto glyph = CCDrawNode::create();
    float gx = T * 0.5f, gy = body + T / 2.f, gl = T * 0.2f;
    glyph->drawSegment({gx - gl, gy}, {gx + gl, gy}, 0.7f, f4(ink));
    if (w.collapsed) glyph->drawSegment({gx, gy - gl}, {gx, gy + gl}, 0.7f, f4(ink));
    node->addChild(glyph, 5);

    w.node = node;
    m_menu->addChild(node);
}

void Overlay::cullWindow(Win& w) {
    if (!w.inner) return;
    auto children = w.inner->getChildren();
    if (!children) return;
    float const top = w.bodyH + w.scroll;          // y of the page origin inside the window
    float const margin = rowH() * 2.f;
    for (auto c : CCArrayExt<CCNode*>(children)) {
        if (c->getTag() == 7731) continue;         // the batched boxes
        float y = top + c->getPositionY();
        c->setVisible(y > -margin && y < w.bodyH + margin);
    }
}

void Overlay::updateBar(Win& w) {
    if (!w.bar) return;
    w.bar->clear();
    float maxScroll = w.contentH - w.bodyH;
    if (maxScroll <= 0.5f) return;
    float barH = std::max(10.f, w.bodyH * w.bodyH / w.contentH);
    float top = w.bodyH - 1.f - (w.bodyH - 2.f - barH) * (w.scroll / maxScroll);
    w.bar->drawSegment({w.width - 1.5f, top}, {w.width - 1.5f, top - barH}, 0.8f, f4(withAlpha(theme().accent, 200)));
}

void Overlay::drawAll() {
    for (size_t k = 0; k < m_order.size(); ++k) {
        auto& w = m_wins[m_order[k]];
        drawWindow(w);
        w.node->setZOrder(static_cast<int>(k));
    }
}

void Overlay::applyPositions() {
    for (auto& w : m_wins) clampWindow(w);
}

void Overlay::saveWindow(Win& w) {
    auto mod = Mod::get();
    auto key = "w-" + w.id;
    w.fx = w.x * menuScale() / m_win.width;
    w.fy = w.y * menuScale() / m_win.height;
    mod->setSavedValue<float>(key + "-x", w.fx);
    mod->setSavedValue<float>(key + "-y", w.fy);
    mod->setSavedValue<bool>(key + "-c", w.collapsed);
}

void Overlay::flushSave() {
    // Positions normally reach the disk when the game closes; if the app is killed in the
    // background they would be lost and the layout would jump back, so write them right away.
    savePositions();
    auto r = Mod::get()->saveData();
    (void)r;
}

void Overlay::savePositions() {
    m_ballFrac = ccp(m_ballPos.x / m_win.width, m_ballPos.y / m_win.height);
    auto mod = Mod::get();
    mod->setSavedValue<float>("ui-ball-x", m_ballFrac.x);
    mod->setSavedValue<float>("ui-ball-y", m_ballFrac.y);
    for (auto& w : m_wins) if (w.placed) saveWindow(w);
}

void Overlay::resetLayout() {
    for (auto& w : m_wins) { w.placed = false; w.collapsed = false; w.scroll = 0.f; }
    autoLayout();
    if (m_open) drawAll();
    savePositions();
}

void Overlay::layout() {
    // the window size changed: keep windows at the same share of the screen
    // Use the remembered share of the screen (kept in memory, never overwritten by clamping), so a
    // temporary size change (leaving and re-opening the app) cannot move the windows for good.
    for (auto& w : m_wins) {
        if (w.fx > kSavedMissing && w.fy > kSavedMissing) {
            w.x = w.fx * m_win.width / menuScale();
            w.y = w.fy * m_win.height / menuScale();
        }
        clampWindow(w);
    }
    placeBall();
}

void Overlay::bringToFront(size_t index) {
    auto it = std::find(m_order.begin(), m_order.end(), index);
    if (it == m_order.end()) return;
    m_order.erase(it);
    m_order.push_back(index);
    for (size_t k = 0; k < m_order.size(); ++k)
        if (auto n = m_wins[m_order[k]].node) n->setZOrder(static_cast<int>(k));
}

void Overlay::resolveOverlap(Win& w) {
    constexpr float gap = 3.f, eps = 0.5f;
    auto heightOf = [this](Win const& v) { return titleH() + (v.collapsed ? 0.f : v.bodyH); };
    float const W = w.width, H = heightOf(w);
    auto clampPos = [&](float& x, float& y) {
        x = std::clamp(x, 0.f, std::max(0.f, menuW() - W));
        y = std::clamp(y, std::min(H, menuH()), std::max(H, menuH()));
    };
    auto hits = [&](float x, float y) {                      // y is the top edge
        for (auto const& o : m_wins) {
            if (&o == &w || !o.placed) continue;
            float oH = heightOf(o);
            if (x < o.x + o.width - eps && x + W > o.x + eps && y - H < o.y - eps && y > o.y - oH + eps) return true;
        }
        return false;
    };
    if (!hits(w.x, w.y)) return;

    float bestX = w.x, bestY = w.y, bestD = 1e18f;
    bool found = false;
    auto consider = [&](float x, float y) {
        clampPos(x, y);
        if (hits(x, y)) return;
        float d = (x - w.x) * (x - w.x) + (y - w.y) * (y - w.y);
        if (d < bestD) { bestD = d; bestX = x; bestY = y; found = true; }
    };
    for (auto const& o : m_wins) {                           // flush against a neighbour first
        if (&o == &w || !o.placed) continue;
        float oH = heightOf(o);
        consider(o.x - W - gap, w.y);
        consider(o.x + o.width + gap, w.y);
        consider(w.x, o.y + gap + H);
        consider(w.x, o.y - oH - gap);
    }
    if (!found) {                                            // otherwise the nearest free spot on a grid
        for (float y = menuH(); y >= H; y -= 8.f)
            for (float x = 0.f; x <= std::max(0.f, menuW() - W); x += 8.f) consider(x, y);
    }
    if (!found) return;                                      // the screen is full: leave it where it is

    w.x = bestX;
    w.y = bestY;
    if (w.node) {
        w.node->stopAllActions();
        w.node->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.2f, ccp(w.x, w.y - H))));
    }
}

void Overlay::openWindow(std::string const& id) {
    for (size_t i = 0; i < m_wins.size(); ++i) {
        if (m_wins[i].id != id) continue;
        m_wins[i].collapsed = false;
        bringToFront(i);
        drawAll();
        saveWindow(m_wins[i]);
        return;
    }
}

void Overlay::toggle() { m_open ? hide() : show(); }

void Overlay::show() {
    m_open = true;
    m_menuAnim->stopAllActions();
    m_menuAnim->setVisible(true);
    m_menu->setScale(menuScale());
    autoLayout();
    for (auto& w : m_wins) clampWindow(w);
    drawAll();
    // unfold from the ball
    m_menuAnim->setPosition(m_ballPos);
    m_menu->setPosition(-m_ballPos.x, -m_ballPos.y);
    m_menuAnim->setScale(0.15f);
    m_menuAnim->runAction(CCEaseBackOut::create(CCScaleTo::create(0.22f, 1.f)));
    pulseBall();
}

void Overlay::hide() {
    m_open = false;
    m_menuAnim->stopAllActions();
    m_menuAnim->setPosition(m_ballPos);
    m_menu->setPosition(-m_ballPos.x, -m_ballPos.y);
    m_menuAnim->runAction(CCSequence::create(
        CCEaseIn::create(CCScaleTo::create(0.14f, 0.15f), 2.f), CCHide::create(), nullptr));
    m_info->removeAllChildren();
    m_infoText.clear();
    pulseBall();
}

void Overlay::refresh() {
    if (!m_open) return;
    if (!m_tappedId.empty()) redrawFor(m_tappedId); else drawAll();   // only the tapped window is rebuilt
}

void Overlay::rebuildAll() {
    if (m_open) drawAll();
}

void Overlay::setInfo(std::string const& text) {
    if (text == m_infoText) return;
    m_infoText = text;
    m_info->removeAllChildren();
    if (text.empty()) return;
    auto const& t = theme();
    float line = kMobile ? 9.5f : 9.f;
    float maxW = std::min(m_win.width - 24.f, kMobile ? 320.f : 360.f);
    auto lines = wrap(text, line, maxW - 12.f, 3);
    float h = static_cast<float>(lines.size()) * (line + 2.f) + 8.f;
    float w = 12.f;
    for (auto const& l : lines) w = std::max(w, textWidth(l, line) + 12.f);
    float x = (m_win.width - w) / 2.f, y = 6.f;
    auto bg = rrect(w, h, 4.f, withAlpha(t.bg, 235));
    bg->setPosition(x, y);
    m_info->addChild(bg);
    for (size_t i = 0; i < lines.size(); ++i)
        m_info->addChild(label(lines[i], line, rgb(t.text), {0.f, 0.5f},
            {x + 6.f, y + h - 4.f - line / 2.f - static_cast<float>(i) * (line + 2.f)}));
    m_infoLeft = 5.f;
}

st
