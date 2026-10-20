#include "Utils/Settings.hpp"
#include <Geode/modify/PlayerObject.hpp>
using namespace geode::prelude;

static auto& nodeath = scro::toggle("Visuals", "nodeath", "No Death Effect", "Skip the death explosion.", false, 20);

class $modify(ScroNoDeath, PlayerObject) {
    void playDeathEffect() {
        if (nodeath.b) return;
        PlayerObject::playDeathEffect();
    }
};
