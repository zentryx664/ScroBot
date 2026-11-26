#include "Utils/Settings.hpp"
#include <Geode/modify/HardStreak.hpp>
using namespace geode::prelude;

static auto& solid = scro::toggle("Player", "solidwave", "Solid Wave Trail", "Draw the wave trail solid.", false, 30);

class $modify(ScroSolidWave, HardStreak) {
    void updateStroke(float dt) {
        if (solid.b) m_isSolid = true;
        HardStreak::updateStroke(dt);
    }
};
