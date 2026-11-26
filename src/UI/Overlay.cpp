#include "UI/Overlay.hpp"
#include "Hacks/FrameStep.hpp"
#include "Macro/Bot.hpp"
#include "UI/Pages.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/MenuLayer.hpp>
#include <algorithm>
#include <cmath>

namespace scro::ui {
using namespace style;

namespace {
Overlay* g_overlay = nullptr;

scro::Option& menuSize = scro::step("Theme", "uiscale", "Menu Size", "Scales every window.", 100.f, 60.f, 250.f, 10.f, "%", 10);
scro::Option& ballSize = scro::step("Theme", "ballsize", "Ball Size", "Size of the floating ball.", 100.f, 70.f, 150.f, 10.f, "%", 20);
scro::Option& ballReset = scro::action("Theme", "ballreset", "Reset Ball",
    "Moves the ball back to where it starts.", [] { if (g_overlay) g_overlay->resetBallPosition(); }, 30);
scro::Option& layoutReset = scro::action("Theme", "layoutreset", "Reset Layout",
    "Puts every window back in its starting place.", [] { if (g_overlay) g_overlay->resetLayout(); }, 40);
bool const wired = ([] {
    menuSize.onChange = [] { if (g_overlay) g_overlay->applyScale(); };
    ballSize.onChange = [] { if (g_overlay) g_overlay->applyBall(); };
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
    auto o = new Overlay();
    if (o->init() && o->setup()) {
        o->autorelease();
        g_overlay = o;
        SceneManager::get()->keepAcrossScenes(o);
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

    m_menu = CCNode::create();
    m_menu->setVisible(false);
    this->addChild(m_menu, 5);

    m_info = CCNode::create();
    this->addChild(m_info, 6);

    m_ball = ball(kSize.ball);
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

void Overlay::placeBall() {
    m_ballPos = ccp(m_ballFrac.x * m_win.width, m_ballFrac.y * m_win.height);
    clampBall();
    m_ball->setScale(ballSize.f / 100.f);
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
        w.width = kSize.width * w.widthFactor;
        auto key = "w-" + w.id;
        float fx = mod->getSavedValue<float>(key + "-x", kSavedMissing);
        float fy = mod->getSavedValue<float>(key + "-y", kSavedMissing);
        w.collapsed = mod->getSavedValue<bool>(key + "-c", false);
        if (fx > kSavedMissing && fy > kSavedMissing) {
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
    float const unit = kSize.width + gap;
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

        w.bar = CCDrawNode::create();
        node->addChild(w.bar, 3);
        updateBar(w);
    }

    // title bar
    auto title = CCDrawNode::create();
    CCPoint tq[4] = {{0.f, body}, {W, body}, {W, H}, {0.f, H}};
    title->drawPolygon(tq, 4, f4(t.accent), 0.f, ccc4f(0, 0, 0, 0));
    node->addChild(title, 4);
    node->addChild(label(w.id, T * 0.62f, rgb(t.onAccent), {0.5f, 0.5f}, {W / 2.f, body + T / 2.f}), 5);
    // collapse glyph: a bar, plus a second bar when collapsed
    auto glyph = CCDrawNode::create();
    float gx = T * 0.5f, gy = body + T / 2.f, gl = T * 0.2f;
    glyph->drawSegment({gx - gl, gy}, {gx + gl, gy}, 0.7f, f4(t.onAccent));
    if (w.collapsed) glyph->drawSegment({gx, gy - gl}, {gx, gy + gl}, 0.7f, f4(t.onAccent));
    node->addChild(glyph, 5);

    w.node = node;
    m_menu->addChild(node);
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

void Overlay::saveWindow(Win const& w) {
    auto mod = Mod::get();
    auto key = "w-" + w.id;
    mod->setSavedValue<float>(key + "-x", w.x * menuScale() / m_win.width);
    mod->setSavedValue<float>(key + "-y", w.y * menuScale() / m_win.height);
    mod->setSavedValue<bool>(key + "-c", w.collapsed);
}

void Overlay::savePositions() {
    m_ballFrac = ccp(m_ballPos.x / m_win.width, m_ballPos.y / m_win.height);
    auto mod = Mod::get();
    mod->setSavedValue<float>("ui-ball-x", m_ballFrac.x);
    mod->setSavedValue<float>("ui-ball-y", m_ballFrac.y);
    for (auto const& w : m_wins) if (w.placed) saveWindow(w);
}

void Overlay::resetLayout() {
    for (auto& w : m_wins) { w.placed = false; w.collapsed = false; w.scroll = 0.f; }
    autoLayout();
    if (m_open) drawAll();
    savePositions();
}

void Overlay::layout() {
    // the window size changed: keep windows at the same share of the screen
    for (auto& w : m_wins) {
        auto key = "w-" + w.id;
        float fx = Mod::get()->getSavedValue<float>(key + "-x", kSavedMissing);
        float fy = Mod::get()->getSavedValue<float>(key + "-y", kSavedMissing);
        if (fx > kSavedMissing && fy > kSavedMissing) {
            w.x = fx * m_win.width / menuScale();
            w.y = fy * m_win.height / menuScale();
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
    m_menu->setVisible(true);
    m_menu->setScale(menuScale());
    autoLayout();
    for (auto& w : m_wins) clampWindow(w);
    drawAll();
}

void Overlay::hide() {
    m_open = false;
    m_menu->setVisible(false);
    m_info->removeAllChildren();
    m_infoText.clear();
}

void Overlay::refresh() {
    if (m_open) drawAll();
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

std::uint64_t Overlay::signature() const {
    auto& bot = Bot::get();
    auto pl = PlayLayer::get();
    bool paused = false;
    if (auto scene = CCDirector::get()->getRunningScene()) paused = scene->getChildByType<PauseLayer>(0) != nullptr;
    std::uint64_t s = 1469598103934665603ULL;
    auto mix = [&](std::uint64_t v) { s = (s ^ v) * 1099511628211ULL; };
    mix(static_cast<std::uint64_t>(bot.state));
    mix(bot.assistedRun);
    mix(bot.actions.size());
    mix(pl != nullptr);
    mix(pl && pl->m_isPracticeMode);
    mix(paused);
    mix(scro::stepper::frozen());
    return s;
}

void Overlay::update(float dt) {
    auto win = CCDirector::get()->getWinSize();
    if (win.width != m_win.width || win.height != m_win.height) {
        m_win = win;
        layout();
        if (m_open) drawAll();
    }
    if (m_infoLeft > 0.f) {
        m_infoLeft -= dt;
        if (m_infoLeft <= 0.f) setInfo("");
    }
    if (!m_open) return;
    m_poll += dt;
    if (m_poll < 0.2f) return;
    m_poll = 0.f;
    auto sig = signature();
    if (sig != m_sig) {
        m_sig = sig;
        drawAll();
    }
}

// ---- touch ---------------------------------------------------------------------------------

int Overlay::windowAt(CCPoint world, bool& onTitle) const {
    auto p = toMenu(world);
    for (size_t k = m_order.size(); k-- > 0;) {
        auto const& w = m_wins[m_order[k]];
        float body = w.collapsed ? 0.f : w.bodyH;
        float H = titleH() + body;
        if (p.x >= w.x && p.x <= w.x + w.width && p.y >= w.y - H && p.y <= w.y) {
            onTitle = p.y >= w.y - titleH();
            return static_cast<int>(m_order[k]);
        }
    }
    return -1;
}

void Overlay::tapWindow(Win& w, CCPoint menuPoint) {
    float body = w.bodyH;
    CCPoint local = {menuPoint.x - w.x, menuPoint.y - (w.y - (titleH() + body))};
    CCPoint u = {local.x, local.y - (body + w.scroll)};
    for (size_t i = w.hits.size(); i-- > 0;) {
        if (w.hits[i].rect.containsPoint(u)) {
            auto fn = w.hits[i].fn;     // the callback redraws the window, so keep our own copy
            if (fn) fn();
            return;
        }
    }
}

bool Overlay::ccTouchBegan(CCTouch* t, CCEvent*) {
    if (m_touching) return false;
    auto world = t->getLocation();
    m_start = m_last = world;
    m_moved = false;
    m_zone = Zone::None;
    m_cur = -1;

    if (ccpDistance(world, m_ballPos) <= ballRadius() + 6.f) {
        m_zone = Zone::Ball;
        m_touching = true;
        m_ball->setScale(ballSize.f / 100.f * 0.92f);
        return true;
    }
    if (!m_open) return false;
    bool onTitle = false;
    int idx = windowAt(world, onTitle);
    if (idx < 0) return false;
    m_touching = true;
    m_cur = idx;
    m_zone = onTitle ? Zone::Title : Zone::Body;
    bringToFront(static_cast<size_t>(idx));
    return true;
}

void Overlay::ccTouchMoved(CCTouch* t, CCEvent*) {
    if (!m_touching) return;
    auto world = t->getLocation();
    if (!m_moved && ccpDistance(world, m_start) > kDragSlop) m_moved = true;
    if (m_moved) {
        if (m_zone == Zone::Ball) {
            m_ballPos = m_ballPos + (world - m_last);
            clampBall();
            m_ball->setPosition(m_ballPos);
        } else if (m_cur >= 0) {
            auto& w = m_wins[static_cast<size_t>(m_cur)];
            auto d = toMenu(world) - toMenu(m_last);
            if (m_zone == Zone::Title) {
                w.x += d.x;
                w.y += d.y;
                clampWindow(w);
                float H = titleH() + (w.collapsed ? 0.f : w.bodyH);
                w.node->setPosition(w.x, w.y - H);
            } else if (w.contentH > w.bodyH) {
                float maxScroll = w.contentH - w.bodyH;
                w.scroll = std::clamp(w.scroll + d.y, 0.f, maxScroll);
                if (w.inner) w.inner->setPositionY(w.bodyH + w.scroll);
                updateBar(w);
            }
        }
    }
    m_last = world;
}

void Overlay::ccTouchEnded(CCTouch*, CCEvent*) {
    if (!m_touching) return;
    auto zone = m_zone;
    int cur = m_cur;
    bool moved = m_moved;
    auto start = m_start;
    m_touching = false;
    m_zone = Zone::None;
    m_cur = -1;
    m_ball->setScale(ballSize.f / 100.f);

    if (zone == Zone::Ball) {
        if (moved) savePositions(); else toggle();
        return;
    }
    if (cur < 0) return;
    auto& w = m_wins[static_cast<size_t>(cur)];
    if (zone == Zone::Title) {
        if (moved) { saveWindow(w); return; }
        auto p = toMenu(start);
        if (p.x - w.x <= titleH() * 1.1f) {      // the collapse glyph
            w.collapsed = !w.collapsed;
            saveWindow(w);
            drawAll();
        }
        return;
    }
    if (!moved) tapWindow(w, toMenu(start));
}

void Overlay::ccTouchCancelled(CCTouch*, CCEvent*) {
    m_touching = false;
    m_zone = Zone::None;
    m_cur = -1;
    m_ball->setScale(ballSize.f / 100.f);
}

// ---- keyboard and wheel (desktop) ------------------------------------------------------------

void Overlay::keyDown(enumKeyCodes key, double) {
    if (key == KEY_Tab) m_tabHeld = true;
    else if (key == KEY_W && m_tabHeld) toggle();
}

void Overlay::keyUp(enumKeyCodes key, double) {
    if (key == KEY_Tab) m_tabHeld = false;
}

bool Overlay::scrollWheel(float y, float x) {
    if (!m_open) return false;
    bool onTitle = false;
    int idx = windowAt(getMousePos(), onTitle);
    if (idx < 0) return false;
    auto& w = m_wins[static_cast<size_t>(idx)];
    if (w.collapsed || w.contentH <= w.bodyH) return false;
    float amount = y != 0.f ? y : x;
    w.scroll = std::clamp(w.scroll + amount * 0.4f, 0.f, w.contentH - w.bodyH);
    if (w.inner) w.inner->setPositionY(w.bodyH + w.scroll);
    updateBar(w);
    return true;
}
}

// The overlay is created once, when the main menu first appears.
class $modify(ScroOverlayMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        scro::ui::Overlay::install();
        return true;
    }
};
