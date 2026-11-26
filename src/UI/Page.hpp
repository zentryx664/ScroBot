#pragma once
#include "UI/Style.hpp"
#include <functional>
#include <vector>

namespace scro::ui {

struct Hit {
    CCRect rect;                 // page coordinates: x to the right, y downward is negative
    std::function<void()> fn;
};

enum class Btn { Normal, Accent, Danger, Quiet };

struct Cell {
    std::string text;
    Btn kind = Btn::Normal;
    std::function<void()> fn;    // empty = shown dimmed and does nothing
};

// A window body built from the top down out of full-width rows.
// It only draws shapes and records which rectangles do what.
class Page {
public:
    explicit Page(float width);

    float width() const { return m_w; }
    float y() const { return m_y; }
    float height() const { return m_y; }
    void skip(float h) { m_y += h; }
    CCNode* node() const { return m_root; }
    std::vector<Hit>& hits() { return m_hits; }

    // low level
    CCDrawNode* box(float x, float top, float w, float h, ccColor4B color);
    CCLabelBMFont* text(std::string const& s, ccColor4B color, float x, float top, float boxH, float anchorX = 0.f, float line = 0.f);
    void tap(float x, float top, float w, float h, std::function<void()> fn);

    // rows
    void rowLabel(std::string const& s, ccColor4B color);                              // not tappable
    void rowHeading(std::string const& s);                                              // small, dim
    void rowAction(std::string const& s, Btn kind, std::function<void()> fn);
    void rowToggle(std::string const& s, bool on, std::function<void()> fn);
    void rowStep(std::string const& s, std::string const& value, std::function<void()> dec, std::function<void()> inc);
    void rowCells(std::vector<Cell> const& cells);                                     // equal buttons side by side
    void rowWithCells(std::string const& s, std::function<void()> fn, std::vector<Cell> const& right, float cellW);
    float rowHeight() const;

private:
    ccColor4B inkFor(Btn kind) const;

    float m_w;
    float m_y = 0.f;
    Ref<CCNode> m_root;
    std::vector<Hit> m_hits;
};
}
