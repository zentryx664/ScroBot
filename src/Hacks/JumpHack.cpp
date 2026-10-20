#include "Utils/Settings.hpp"
#include <Geode/modify/PlayerObject.hpp>
using namespace geode::prelude;

static auto& jump = scro::toggle("Player", "jump", "Jump Hack", "Jump in mid-air.", false, 10);

class $modify(ScroJump, PlayerObject) {
    void update(float dt) {
        if (jump.b && PlayLayer::get()) m_isOnGround = true;
        PlayerObject::update(dt);
    }
};
