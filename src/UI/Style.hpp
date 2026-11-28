#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <vector>
using namespace geode::prelude;

// Colours, sizes and the few shapes the menu is drawn with. Nothing here uses game textures.
namespace scro::style {

struct Theme {
    char const* name;
    ccColor4B bg, card, line, text, muted, accent, onAccent, off;
};

int themeCount();
Theme const& themeAt(int index);
Theme const& theme();
int themeIndex();
void setTheme(int index);

#ifdef GEODE_IS_MOBILE
inline constexpr bool kMobile = true;
#else
inline constexpr bool kMobile = false;
#endif

// All sizes are in points. Desktop is a dense, large-screen layout; phones get a smaller menu
// with taller rows so they are easy to tap.
struct Size {
    float title, row, line, pad, gap, width, ball;
};
inline constexpr Size kSize = kMobile
    ? Size{19.f, 18.f, 9.5f, 4.f, 1.f, 112.f, 22.f}
    : Size{12.f, 10.5f, 7.4f, 3.f, 1.f, 80.f, 30.f};

// "Resize" (Theme): scales tab widths, rows and buttons. 1.0 = default.
float tabScale();
inline float rowH() { return kSize.row * tabScale(); }
inline float lineH() { return kSize.line * tabScale(); }
inline float titleHeight() { return kSize.title * tabScale(); }

// "Gradient" (Theme): colours the window title bars. gradientIndex() == 0 means off.
int gradientIndex();
int gradientCount();
CCDrawNode* gradientBar(float w, float h);          // horizontal gradient rectangle (origin bottom-left)

inline constexpr char const* kFont = "chatFont.fnt";

ccColor3B rgb(ccColor4B c);
ccColor4F f4(ccColor4B c);
ccColor4B withAlpha(ccColor4B c, GLubyte a);

CCDrawNode* rrect(float w, float h, float radius, ccColor4B fill);
CCDrawNode* disc(float radius, ccColor4B fill);
CCDrawNode* ball(float radius);                    // purple ring menu button
CCNode* ballButton(float radius);                  // ScroBot floating button sprite
CCDrawNode* cross(float size, float thickness, ccColor4B color);

// `line` is the height of one text line in points.
float scaleFor(float line);
float textWidth(std::string const& s, float line);
CCLabelBMFont* label(std::string const& s, float line, ccColor3B color, CCPoint anchor, CCPoint pos);
std::vector<std::string> wrap(std::string const& s, float line, float maxWidth, size_t maxLines);
}
