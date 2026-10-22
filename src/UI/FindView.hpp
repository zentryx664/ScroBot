#pragma once
#include "Utils/Settings.hpp"
#include <Geode/Geode.hpp>
#include <functional>
#include <vector>
using namespace geode::prelude;

namespace scro {
// Global > Find. Search is explicit (press Search) so the list is not rebuilt
// on every keystroke; result rows use the same chatFont/theme as the main menu.
class FindView : public CCNode {
    float m_w = 0.f, m_h = 0.f;
    TextInput* m_input = nullptr;
    std::string m_query;
    std::vector<Option*> m_results;
    std::vector<Option*> m_visible;
    int m_page = 0;
    std::function<void(Option*)> m_onSelect;
    bool setup(float w, float h, std::function<void(Option*)> onSelect);
    void rebuild();
    void runSearch();
public:
    static FindView* create(float w, float h, std::function<void(Option*)> onSelect);
    void onSearch(CCObject*);
    void onPage(CCObject*);
    void onResult(CCObject*);
};
}
