#include "UI/Style.hpp"
#include <algorithm>
#include <cmath>

namespace scro::style {
namespace {
constexpr float kPi = 3.14159265f;

Theme const kThemes[] = {
    {"Violet", {24, 24, 26, 250}, {40, 40, 43, 255}, {66, 66, 72, 255}, {255, 255, 255, 255},
        {156, 156, 164, 255}, {176, 96, 255, 255}, {255, 255, 255, 255}, {74, 74, 80, 255}},
    {"Midnight", {16, 18, 24, 246}, {28, 31, 40, 255}, {54, 58, 72, 255}, {236, 239, 246, 255},
        {140, 147, 164, 255}, {96, 165, 250, 255}, {8, 14, 26, 255}, {62, 67, 82, 255}},
    {"Mono", {0, 0, 0, 246}, {20, 20, 20, 255}, {64, 64, 64, 255}, {255, 255, 255, 255},
        {160, 160, 160, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}, {64, 64, 64, 255}},
    {"Paper", {246, 247, 250, 248}, {255, 255, 255, 255}, {218, 222, 230, 255}, {28, 32, 44, 255},
        {110, 118, 135, 255}, {37, 99, 235, 255}, {255, 255, 255, 255}, {200, 205, 216, 255}},
    {"Ocean", {10, 25, 35, 246}, {16, 39, 55, 255}, {32, 66, 88, 255}, {226, 244, 250, 255},
        {120, 162, 182, 255}, {45, 212, 191, 255}, {4, 26, 28, 255}, {40, 72, 92, 255}},
    {"Forest", {14, 24, 18, 246}, {22, 38, 28, 255}, {44, 72, 52, 255}, {230, 245, 234, 255},
        {132, 162, 140, 255}, {134, 239, 172, 255}, {8, 30, 16, 255}, {48, 74, 56, 255}},
    {"Rose", {28, 16, 22, 246}, {44, 26, 34, 255}, {80, 48, 62, 255}, {250, 234, 240, 255},
        {178, 136, 152, 255}, {244, 114, 182, 255}, {40, 8, 24, 255}, {82, 52, 66, 255}},
};
constexpr int kThemeCount = static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0]));
int g_theme = -1;

// one hidden label used to measure text
CCLabelBMFont* meter() {
    static Ref<CCLabelBMFont> m = CCLabelBMFont::create("Ag", kFont);
    return m;
}
float lineHeightAtOne() {
    static float h = 0.f;
    if (h <= 0.f) {
        auto m = meter();
        m->setString("Ag");
        h = std::max(1.f, m->getContentSize().height);
    }
    return h;
}
}

int themeCount() { return kThemeCount; }
Theme const& themeAt(int i) { return kThemes[std::clamp(i, 0, kThemeCount - 1)]; }
int themeIndex() {
    if (g_theme < 0) g_theme = std::clamp(Mod::get()->getSavedValue<int>("ui-theme", 0), 0, kThemeCount - 1);
    return g_theme;
}
Theme const& theme() { return kThemes[themeIndex()]; }
void setTheme(int i) {
    g_theme = std::clamp(i, 0, kThemeCount - 1);
    Mod::get()->setSavedValue<int>("ui-theme", g_theme);
}

ccColor3B rgb(ccColor4B c) { return {c.r, c.g, c.b}; }
ccColor4F f4(ccColor4B c) { return ccc4f(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f); }
ccColor4B withAlpha(ccColor4B c, GLubyte a) { return {c.r, c.g, c.b, a}; }

CCDrawNode* rrect(float w, float h, float radius, ccColor4B fill) {
    auto n = CCDrawNode::create();
    float r = std::max(0.f, std::min({radius, w / 2.f, h / 2.f}));
    std::vector<CCPoint> pts;
    constexpr int seg = 6;
    auto arc = [&](float cx, float cy, float start) {
        for (int i = 0; i <= seg; ++i) {
            float a = start + (kPi / 2.f) * static_cast<float>(i) / seg;
            pts.push_back({cx + r * std::cos(a), cy + r * std::sin(a)});
        }
    };
    arc(w - r, h - r, 0.f);
    arc(r, h - r, kPi / 2.f);
    arc(r, r, kPi);
    arc(w - r, r, kPi * 1.5f);
    n->drawPolygon(pts.data(), static_cast<unsigned int>(pts.size()), f4(fill), 0.f, ccc4f(0, 0, 0, 0));
    n->setContentSize({w, h});
    return n;
}

CCDrawNode* disc(float radius, ccColor4B fill) {
    auto n = CCDrawNode::create();
    n->drawDot({radius, radius}, radius, f4(fill));
    n->setContentSize({radius * 2.f, radius * 2.f});
    return n;
}

CCDrawNode* ball(float radius) {
    auto n = CCDrawNode::create();
    float const r = radius;
    n->drawDot({r, r}, r, ccc4f(1.f, 1.f, 1.f, 0.55f));          // soft rim
    float const inner = r - 1.5f;
    constexpr int slices = 30;
    for (int i = 0; i < slices; ++i) {
        float y0 = -inner + 2.f * inner * static_cast<float>(i) / slices;
        float y1 = -inner + 2.f * inner * static_cast<float>(i + 1) / slices + 0.4f;
        float mid = (y0 + y1) * 0.5f;
        float half = std::sqrt(std::max(0.f, inner * inner - mid * mid));
        float shade = (static_cast<float>(i) + 0.5f) / slices;   // bottom black, top white
        CCPoint q[4] = {{r - half, r + y0}, {r + half, r + y0}, {r + half, r + y1}, {r - half, r + y1}};
        n->drawPolygon(q, 4, ccc4f(shade, shade, shade, 1.f), 0.f, ccc4f(0, 0, 0, 0));
    }
    n->setContentSize({r * 2.f, r * 2.f});
    return n;
}

CCDrawNode* cross(float size, float thickness, ccColor4B color) {
    auto n = CCDrawNode::create();
    n->drawSegment({0.f, 0.f}, {size, size}, thickness / 2.f, f4(color));
    n->drawSegment({0.f, size}, {size, 0.f}, thickness / 2.f, f4(color));
    n->setContentSize({size, size});
    return n;
}

float scaleFor(float line) { return line / lineHeightAtOne(); }

float textWidth(std::string const& s, float line) {
    auto m = meter();
    m->setString(s.c_str());
    return m->getContentSize().width * scaleFor(line);
}

CCLabelBMFont* label(std::string const& s, float line, ccColor3B color, CCPoint anchor, CCPoint pos) {
    auto l = CCLabelBMFont::create(s.c_str(), kFont);
    l->setScale(scaleFor(line));
    l->setColor(color);
    l->setAnchorPoint(anchor);
    l->setPosition(pos);
    return l;
}

std::vector<std::string> wrap(std::string const& s, float line, float maxWidth, size_t maxLines) {
    std::vector<std::string> out;
    std::string current, word;
    bool cut = false;
    auto flushWord = [&] {
        if (word.empty()) return;
        std::string trial = current.empty() ? word : current + " " + word;
        if (current.empty() || textWidth(trial, line) <= maxWidth) {
            current = trial;
        } else if (out.size() + 1 >= maxLines) {
            cut = true;
        } else {
            out.push_back(current);
            current = word;
        }
        word.clear();
    };
    for (char c : s) {
        if (cut) break;
        if (c == ' ') flushWord(); else word += c;
    }
    if (!cut) flushWord();
    if (!current.empty()) out.push_back(cut ? current + "..." : current);
    return out;
}
}
