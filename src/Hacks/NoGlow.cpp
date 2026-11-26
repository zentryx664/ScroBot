#include "Hacks/FpsBoost.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/GameObject.hpp>
using namespace geode::prelude;

static auto& noglow = scro::toggle("Visuals", "noglow", "No Glow", "Remove glow (next level load).", false, 30);

class $modify(ScroNoGlow, GameObject) {
    void addGlow(std::string frame) {
        if (noglow.b || scro::fps::boostFrames()) return;
        GameObject::addGlow(frame);
    }
};
