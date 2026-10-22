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

}
