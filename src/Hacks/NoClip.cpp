#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

static auto& noclip = scro::toggle("Global", "noclip", "No Clip", "Ignore hazards. Forces Safe Mode.", false, 20);

class $modify(ScroNoClip, PlayLayer) {
    void destroyPlayer(PlayerObject* p, GameObject* o) {
        if (noclip.b && o != m_anticheatSpike) return;
        PlayLayer::destroyPlayer(p, o);
    }
};
