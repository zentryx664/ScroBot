#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

static auto& sync = scro::toggle("Level", "musicsync", "Practice Music Sync", "Resync music on practice respawn.", false, 30);

class $modify(ScroMusicSync, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        if (sync.b && m_isPracticeMode && m_gameState.m_levelTime > 0.0)
            FMODAudioEngine::sharedEngine()->setMusicTimeMS(
                static_cast<unsigned int>(m_gameState.m_levelTime * 1000.0), true, 0);
    }
};
