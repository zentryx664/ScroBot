#pragma once
#include <string>

// Level screen recorder (Android). The game frame is copied on the GPU into a hardware video encoder,
// game sound is tapped from FMOD, and the finished mp4 is saved to the gallery (Movies/ScroBot).
// No extra threads are used: everything runs from the game's own frame loop.
namespace scro::rec {
struct Config {
    int fps = 30;       // frames per second written to the video
    int height = 720;   // output height in pixels (width follows the screen shape)
};
bool start(Config const& cfg, std::string& error);
void frame();                    // call once after the game has drawn a frame
std::string stop();              // returns where the video was saved ("" on failure)
bool active();
}
