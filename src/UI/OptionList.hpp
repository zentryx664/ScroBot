#pragma once
#include "Utils/Settings.hpp"
#include <Geode/Geode.hpp>
#include <functional>
using namespace geode::prelude;

namespace scro {
// All options of one category as rows, grouped under section headings.
class OptionList : public CCNode {
    float m_w = 0, m_h = 0;
    std::vector<Option*> m_opts;
    std::vector<CCLabelBMFont*> m_vals;
    std::function<void(Option*)> m_onSel;
    bool setup(std::string const& cat, float w, float availH, bool animate, std::function<void(Option*)> onSel);
public:
    static OptionList* create(std::string const& cat, float w, float availH, bool animate, std::function<void(Option*)> onSel);
    float height() const { return m_h; }
    void onToggle(CCObject*);
    void onStep(CCObject*);
    void onAction(CCObject*);
};
}
