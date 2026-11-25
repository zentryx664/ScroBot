#pragma once

// Click Analyzer: shows where each click lands inside the game's frame window
// (frame -> physics steps), how long it waited, and a running summary.
namespace scro::clicks {
void frameBegin(float dt);   // start of a rendered frame (the game is about to run its physics steps)
void frameEnd();             // end of that frame
void step(bool halfTick);    // one physics step (or half step) is about to run
void arrive();               // a tap reached the game's input queue (not available on PC)
void applied(bool down);     // the game applied a press or release on the current step
void reset();                // clear the statistics
}
