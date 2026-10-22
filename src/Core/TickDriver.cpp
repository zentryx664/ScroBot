#include "Core/Tick.hpp"
#include "Hacks/FpsBoost.hpp"
#include "Hacks/FrameStep.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
using namespace geode::prelude;

// The single update hook: Freeze / Frame Step, Click Precision, and every registered tick.
class $modify(ScroTickLayer, GJBaseGameLayer) {
    void releaseHeld(scro::fps::Held const& h) {
        scro::fps::g_bypass = true;
        GJBaseGameLayer::queueButton(h.button, h.down, h.p1, 0.0);
        scro::fps::g_bypass = false;
    }
    void update(float dt) {
        auto pl = PlayLayer::get();
        if (!pl) return GJBaseGameLayer::update(dt);

        if (scro::stepper::frozen()) {
            if (scro::fps::pending()) for (auto& h : scro::fps::drain()) releaseHeld(h);
            if (scro::stepper::consume()) GJBaseGameLayer::update(1.f / 60.f);
            scro::tick::run(pl, dt, true);
            return;
        }
        if (scro::fps::precise()) {
            // split the frame around each click so it lands on the exact physics step
            float done = 0.f;
            for (auto& h : scro::fps::drain()) {
                if (h.frac > done) { GJBaseGameLayer::update(dt * (h.frac - done)); done = h.frac; }
                releaseHeld(h);
            }
            GJBaseGameLayer::update(dt * (1.f - done));
        } else {
            if (scro::fps::pending()) for (auto& h : scro::fps::drain()) releaseHeld(h);
            GJBaseGameLayer::update(dt);
        }
        scro::tick::run(pl, dt, false);
    }
};
