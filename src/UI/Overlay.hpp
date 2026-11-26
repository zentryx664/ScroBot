#pragma once
#include "UI/Page.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace scro::ui {

// A node that only draws inside its own rectangle (used for scrolling window bodies).
class ClipNode : public CCNode {
public:
    static ClipNode* create(CCSize size);
    void visit() override;
};

// The floating ball and the set of windows it shows. It lives above every screen.
class Overlay : public CCLayer {
public:
    static Overlay* get();
    static void install();

    void toggle();
    void show();
    void hide();
    bool isOpen() const { return m_open; }
    void refresh();                         // redraw every window, keeping positions and scroll
    void rebuildAll();                      // after a theme change
    void applyScale();
    void applyBall();
    void resetBallPosition();
    void resetLayout();
    void openWindow(std::string const& id); // expand a window and bring it to the front
    void setInfo(std::string const& text);  // line shown at the bottom of the screen

    bool ccTouchBegan(CCTouch* t, CCEvent* e) override;
    void ccTouchMoved(CCTouch* t, CCEvent* e) override;
    void ccTouchEnded(CCTouch* t, CCEvent* e) override;
    void ccTouchCancelled(CCTouch* t, CCEvent* e) override;
    void keyDown(enumKeyCodes key);
    void keyDown(enumKeyCodes key, double timestamp);
    void keyUp(enumKeyCodes key);
    void keyUp(enumKeyCodes key, double timestamp);
    bool scrollWheel(float y, float x) override;
    void update(float dt) override;

private:
    struct Win {
        std::string id;
        float widthFactor = 1.f;
        std::function<void(Page&)> build;
        float width = 0.f;
        float x = 0.f, y = 0.f;             // top-left corner, in menu coordinates
        bool collapsed = false;
        bool placed = false;                // has a position (saved or computed)
        float scroll = 0.f;
        float contentH = 0.f, bodyH = 0.f;
        CCNode* node = nullptr;
        CCNode* inner = nullptr;
        CCDrawNode* bar = nullptr;
        std::vector<Hit> hits;
    };
    enum class Zone { None, Ball, Title, Body };

    bool setup();
    void makeWindows();
    void measure(Win& w);
    void autoLayout();
    void drawWindow(Win& w);
    void updateBar(Win& w);
    void drawAll();
    void applyPositions();
    void layout();
    void saveWindow(Win const& w);
    void savePositions();
    void placeBall();
    void clampBall();
    void clampWindow(Win& w);
    void bringToFront(size_t index);
    float menuScale() const;
    float ballRadius() const;
    float titleH() const { return style::kSize.title; }
    float maxBodyH() const;
    float menuW() const;
    float menuH() const;
    int windowAt(CCPoint world, bool& onTitle) const;
    CCPoint toMenu(CCPoint world) const;
    void tapWindow(Win& w, CCPoint menuPoint);
    std::uint64_t signature() const;

    std::vector<Win> m_wins;
    std::vector<size_t> m_order;            // back to front
    CCNode* m_menu = nullptr;               // scaled container for the windows
    CCNode* m_ball = nullptr;
    CCNode* m_info = nullptr;
    std::string m_infoText;
    float m_infoLeft = 0.f;

    bool m_open = false;
    CCSize m_win;
    CCPoint m_ballPos, m_ballFrac{0.05f, 0.6f};

    bool m_touching = false, m_moved = false;
    Zone m_zone = Zone::None;
    int m_cur = -1;
    CCPoint m_start, m_last;
    bool m_tabHeld = false;
    float m_poll = 0.f;
    std::uint64_t m_sig = 0;
};
}
