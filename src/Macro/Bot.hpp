#pragma once
#include <Geode/Geode.hpp>
#include <filesystem>
#include <vector>
using namespace geode::prelude;

namespace scro {
struct PState { float x, y, rot, yv; };
struct Action {
    int32_t frame, button;
    uint8_t down, p1, hasState, pad;
    PState a, b;   // player 1 / 2 snapshot taken at the input
};
static_assert(sizeof(Action) == 44, "layout must match on 32/64-bit");
enum class BotState { Idle, Recording, Playing };

class Bot {
public:
    BotState state = BotState::Idle;
    std::vector<Action> actions;
    size_t index = 0;
    bool injecting = false;
    // Remains true for the entire run after macro playback has affected it,
    // even if playback reaches the end before the level completes.
    bool assistedRun = false;
    static Bot& get();
    static std::filesystem::path dir();
    bool save(std::string const& name);
    bool load(std::string const& name);
    void seek(int frame);
};
PState grab(PlayerObject* p);
void apply(PlayerObject* p, PState const& s);
}
