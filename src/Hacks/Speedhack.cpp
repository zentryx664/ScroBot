#include "Utils/Settings.hpp"
#include <Geode/modify/CCScheduler.hpp>
using namespace geode::prelude;

static auto& speed = scro::step("Global", "speed", "Speedhack", "Scales game time.", 1.f, 0.1f, 3.f, 0.05f, "x", 10);
static bool const speedSection = (speed.section = "Speedhack", true);

class $modify(ScroSpeed, CCScheduler) {
    void update(float dt) {
        if (speed.f != 1.f && PlayLayer::get()) dt *= speed.f;
        CCScheduler::update(dt);
    }
};
