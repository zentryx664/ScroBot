#pragma once
#include <Geode/Geode.hpp>
#include <filesystem>
#include <string>
#include <vector>
using namespace geode::prelude;

namespace scro {
struct PState { float x, y, rot, yv; };

// One recorded input. `frame` is the physics step it happened on.
struct Action {
    int32_t frame, button;
    uint8_t down, p1, hasState, pad;
    PState a, b;   // player 1 and player 2 position at the input (used to correct drift)
};
static_assert(sizeof(Action) == 44, "layout must match on 32 and 64 bit");

enum class BotState { Idle, Recording, Playing };

struct MacroInfo {
    std::string name;
    uint32_t inputs = 0;
};

class Bot {
public:
    BotState state = BotState::Idle;
    std::vector<Action> actions;
    size_t index = 0;
    bool injecting = false;
    std::string name;            // macro currently loaded, empty if it was never saved

    // Stays true for the rest of the run once playback has touched it, even after playback ends,
    // so a run that was helped by a macro is never counted as a clean completion.
    bool assistedRun = false;

    static Bot& get();
    static std::filesystem::path dir();

    bool save(std::string const& name);
    bool load(std::string const& name);
    bool remove(std::string const& name);
    std::vector<MacroInfo> list();
    std::string suggestName();   // unused name based on the current level
    int lastFrame() const;
    void seek(int frame);
};

PState grab(PlayerObject* p);
void apply(PlayerObject* p, PState const& s);
}
