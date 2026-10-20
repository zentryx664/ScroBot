#include "Draw.hpp"
#include "UI/Theme.hpp"

namespace scro::draw {
CCLayerColor* rect(float w, float h, ccColor4B c) {
    auto r = CCLayerColor::create(c, w, h);
    r->ignoreAnchorPointForPosition(false);
    r->setAnchorPoint({0.f, 0.f});
    return r;
}
CCLabelBMFont* label(std::string const& t, float scale, ccColor3B col, CCPoint anchor, CCPoint pos) {
    auto l = CCLabelBMFont::create(t.c_str(), "chatFont.fnt");
    l->setScale(scale); l->setColor(col); l->setAnchorPoint(anchor); l->setPosition(pos);
    return l;
}

template <class T> static bool fadeAs(CCNode* n, float delay) {
    if (auto p = typeinfo_cast<T*>(n)) {
        GLubyte o = p->getOpacity();
        p->setOpacity(0);
        n->runAction(CCSequence::create(CCDelayTime::create(delay), CCFadeTo::create(0.12f, o), nullptr));
        return true;
    }
    return false;
}
void fadeTree(CCNode* n, float delay) {
    if (fadeAs<CCLabelBMFont>(n, delay)) return;
    if (!fadeAs<CCLayerColor>(n, delay)) fadeAs<CCSprite>(n, delay);
    if (auto ch = n->getChildren())
        for (auto c : CCArrayExt<CCNode*>(ch)) fadeTree(c, delay);
}

CCNode* button(char const* text, float w, float h) {
    auto n = CCNode::create();
    n->setContentSize({w, h});
    n->addChild(rect(w, h, ccc4(30, 30, 32, 255)));
    auto l = label(text, 0.7f, theme::TEXT, {0.5f, 0.5f}, {w / 2, h / 2});
    l->limitLabelWidth(w - 6, 0.7f, 0.3f);
    n->addChild(l);
    return n;
}

void triangle(CCNode* parent, bool down, float s, CCPoint pos, ccColor3B c) {
    auto d = CCDrawNode::create();
    CCPoint v[3];
    if (down) { v[0] = {-s, s * 0.6f}; v[1] = {s, s * 0.6f}; v[2] = {0.f, -s * 0.6f}; }
    else      { v[0] = {-s * 0.6f, s}; v[1] = {-s * 0.6f, -s}; v[2] = {s * 0.6f, 0.f}; }
    d->drawPolygon(v, 3, ccc4f(c.r / 255.f, c.g / 255.f, c.b / 255.f, 1.f), 0.f, ccc4f(0, 0, 0, 0));
    d->setPosition(pos);
    parent->addChild(d);
}

CCNode* panel(float w, float h, float titleH) {
    auto a = theme::accent();
    float k = scro::find("menuopacity").f / 100.f;
    auto n = CCNode::create();
    n->setContentSize({w, h});
    auto bd = rect(w + 2, h + 2, ccc4(32, 32, 34, 255)); bd->setPosition(-1, -1); n->addChild(bd);
    n->addChild(rect(w, h, ccc4(13, 13, 14, static_cast<int>(248 * k))));
    auto t = rect(w, titleH, ccc4(a.r, a.g, a.b, 255)); t->setPosition(0, h - titleH); n->addChild(t);
    return n;
}
}
