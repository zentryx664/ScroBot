#include "Utils/Settings.hpp"
#include <Geode/modify/GameObject.hpp>
using namespace geode::prelude;

static auto& noglow = scro::toggle("Visuals", "noglow", "No Glow", "Remove glow (next level load).", false, 30);

class $modify(ScroNoGlow, GameObject) {
    void addGlow(gd::string frame) {
        if (noglow.b) return;
        GameObject::addGlow(frame);
    }
};
