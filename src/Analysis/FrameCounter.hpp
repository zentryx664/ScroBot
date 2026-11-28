#pragma once
#include <Geode/Geode.hpp>
using namespace geode::prelude;

// Frame Counter: reads the timing window of every click, like a macro that also measures its inputs.
// The window of a click is the number of physics frames of room it has before the next / previous
// input of the run (1 = the tightest, 9-12 = relaxed). Each click is counted in a coloured bucket,
// marked with a square at the player's position, and plays a sound: fewer frames = lower pitch.
namespace scro::fc {
bool enabled();
void frameBegin(float dt);   // kept so the callers stay simple; the counter does not need them
void frameEnd();
void step(bool halfTick);
void arrive();
void pressed(int window, PlayerObject* player);   // a press was applied, with its measured window
void reset();                                     // clear the counters (start of a fresh run)
}
