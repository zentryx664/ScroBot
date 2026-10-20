#pragma once
#include <Geode/Geode.hpp>
#include <functional>
using namespace geode::prelude;

namespace scro {
class MacroView : public CCNode {
    float m_w = 0, m_h = 0;
    bool m_d = false;
    std::function<void()> m_onStart;
    TextInput* m_input = nullptr;
    std::vector<std::string> m_names;
    int m_page = 0, m_sel = -1;
    bool setup(float w, bool desk, std::function<void()> onStart);
    void rebuild();
    void begin(int state);
public:
    static MacroView* create(float w, bool desk, std::function<void()> onStart);
    float height() const { return m_h; }
    void onRecord(CCObject*);
    void onPlay(CCObject*);
    void onSave(CCObject*);
    void onPick(CCObject*);
    void onPage(CCObject*);
};
}
