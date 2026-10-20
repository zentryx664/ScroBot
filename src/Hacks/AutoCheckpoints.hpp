#include "Core/Tick.hpp"
#include "Utils/Settings.hpp"
using namespace geode::prelude;

static auto& autocp = scro::toggle("Level", "autocp", "Auto Checkpoints", "Checkpoint every 1.5 s in practice.", false, 10);
static float g_t = 0.f;

static scro::tick::Reg reg(+[](PlayLayer* pl, float dt, bool frozen) {
    if (!autocp.b || frozen || !pl->m_isPracticeMode || !pl->m_player1 || pl->m_player1->m_isDead) { g_t = 0.f; return; }
    g_t += dt;
    if (g_t >= 1.5f) { g_t = 0.f; pl->markCheckpoint(); }
});
