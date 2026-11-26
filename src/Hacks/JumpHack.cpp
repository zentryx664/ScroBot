#include "Utils/Settings.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
using namespace geode::prelude;

static auto& jump = scro::toggle("Player", "jump", "Jump Hack", "Jump in mid-air. Completions count as test runs.", false, 10);

namespace {
bool g_holding = false;   // the jump button is currently held
}

// Why the old version could not jump: it set "on ground" inside PlayerObject::update, but the game reads
// that flag earlier, while it processes the tap. The flag was still false (we were in the air), so the tap did
// nothing. Now the flag is raised at the moment the tap is handled, and kept up while the button is held so
// holding the button keeps jumping, like on the ground.
class $modify(ScroJumpInput, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (button == 1 && PlayLayer::get()) {
            g_holding = down;
            if (down && jump.b) {
                if (m_player1) m_player1->m_isOnGround = true;
                if (m_player2) m_player2->m_isOnGround = true;
            }
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};

class $modify(ScroJump, PlayerObject) {
    void update(float dt) {
        if (jump.b && g_holding && PlayLayer::get()) m_isOnGround = true;
        PlayerObject::update(dt);
    }
};
