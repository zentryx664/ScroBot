#include "Utils/Settings.hpp"
#include <Geode/modify/CCParticleSystem.hpp>
using namespace geode::prelude;

static auto& nop = scro::toggle("Visuals", "noparticles", "No Particles", "Hide particle effects.", false, 40);

class $modify(ScroNoParticles, CCParticleSystem) {
    void update(float dt) {
        if (nop.b && PlayLayer::get()) this->setVisible(false);
        else if (!this->isVisible() && PlayLayer::get()) this->setVisible(true);
        CCParticleSystem::update(dt);
    }
};
