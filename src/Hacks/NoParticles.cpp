#include "Hacks/FpsBoost.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/CCParticleSystem.hpp>
using namespace geode::prelude;

static auto& nop = scro::toggle("Visuals", "noparticles", "No Particles", "Hide particle effects.", false, 40);

class $modify(ScroNoParticles, CCParticleSystem) {
    struct Fields { bool hiddenByUs = false; };

    void update(float dt) {
        if (PlayLayer::get()) {
            static auto* smoothOpt = &scro::find("smooth");
            bool want = nop.b || smoothOpt->b || scro::fps::boostFrames();
            // only undo what this mod hid, so particles the game keeps hidden itself stay hidden
            if (want && this->isVisible()) { this->setVisible(false); m_fields->hiddenByUs = true; }
            else if (!want && m_fields->hiddenByUs) { this->setVisible(true); m_fields->hiddenByUs = false; }
        }
        CCParticleSystem::update(dt);
    }
};
