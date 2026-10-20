#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

static auto& ap = scro::toggle("Level", "autopractice", "Auto Practice", "Start every level in practice mode.", false, 20);

class $modify(ScroAutoPractice, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        if (ap.b && !m_isPracticeMode) this->togglePracticeMode(true);
    }
};
