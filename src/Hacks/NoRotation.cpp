#include "Utils/Settings.hpp"
#include <Geode/modify/PlayerObject.hpp>
using namespace geode::prelude;

static auto& norot = scro::toggle("Player", "norot", "No Rotation", "Keep the icon from spinning.", false, 20);

class $modify(ScroNoRot, PlayerObject) {
    void updateRotation(float t) {
        if (norot.b && PlayLayer::get()) return;
        PlayerObject::updateRotation(t);
    }
};
