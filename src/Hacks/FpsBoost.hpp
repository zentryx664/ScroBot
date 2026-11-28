#pragma once
#include <vector>
namespace scro::fps {
struct Held { int button; bool down; bool p1; float frac; };   // frac = how far into the frame it happened (0..1)
inline bool g_bypass = false;     // true while we release a held input ourselves

void initOptions();              // register FPS options after Geode is loaded
bool precise();                   // FPS Boost + Click Precision are on
bool pending();                   // inputs are being held
void hold(int button, bool down, bool p1);
std::vector<Held> drain();        // take held inputs, with their position inside this frame
void apply();                     // push FPS settings to the game
void enforce();                   // re-apply if the game changed the frame rate behind our back
bool boostFrames();               // FPS Boost + Boost Frames are on
}
