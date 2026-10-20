#include "Labels/Labels.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

static auto& linfo = scro::toggle("Level", "levelinfo", "Show Level Info", "Name, author and ID at the start.", true, 40);

class $modify(ScroLevelInfo, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        if (!linfo.b || !m_level) return;
        auto l = scro::labels::info(this);
        l->setString(fmt::format("{}  by  {}   #{}", m_level->m_levelName.c_str(),
            m_level->m_creatorName.c_str(), m_level->m_levelID.value()).c_str());
        l->setOpacity(0);
        l->runAction(CCSequence::create(CCDelayTime::create(0.4f), CCFadeTo::create(0.4f, 235),
            CCDelayTime::create(2.5f), CCFadeOut::create(0.6f), nullptr));
    }
};
