#include "Notifications/Notify.hpp"
#include "Record/Recorder.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/CCDirector.hpp>
using namespace geode::prelude;

#ifdef GEODE_IS_ANDROID
static scro::Option& opt = scro::toggle("Global", "screenrec", "Screen Record",
    "Records levels with game sound into your gallery (Movies/ScroBot). Only runs inside a level.", false, 30);
static scro::Option& fpsOpt = scro::step("Global", "recfps", "Record FPS",
    "Frames per second of the video (30 or higher).", 0.f, 0.f, 2.f, 1.f, "", 31);
static scro::Option& qualOpt = scro::step("Global", "recq", "Record Quality",
    "Video height. Lower is lighter on weak phones.", 1.f, 0.f, 2.f, 1.f, "", 32);

static PlayLayer* g_failedFor = nullptr;       // do not retry endlessly in the same level
static bool _setup = ([] {
    fpsOpt.names = {"30", "45", "60"};
    qualOpt.names = {"480p", "720p", "1080p"};
    for (auto* o : {&opt, &fpsOpt, &qualOpt}) o->section = "Recording";
    opt.onChange = [] { g_failedFor = nullptr; };
}(), true);

static void recordTick() {
    auto pl = PlayLayer::get();
    if (!opt.b || !pl) {
        if (scro::rec::active()) {
            auto where = scro::rec::stop();
            if (where.empty()) scro::notify("Recording could not be saved", false);
            else scro::notify("Saved: " + where);
        }
        if (!pl) g_failedFor = nullptr;
        return;
    }
    if (!scro::rec::active() && pl != g_failedFor) {
        static int const fps[] = {30, 45, 60}, height[] = {480, 720, 1080};
        scro::rec::Config cfg;
        cfg.fps = fps[std::clamp(static_cast<int>(fpsOpt.f), 0, 2)];
        cfg.height = height[std::clamp(static_cast<int>(qualOpt.f), 0, 2)];
        std::string err;
        if (scro::rec::start(cfg, err)) scro::notify("Recording started");
        else { scro::notify("Recording failed: " + err, false); g_failedFor = pl; }
    }
    scro::rec::frame();
}

// runs right after the game finished drawing a frame, before it is shown
class $modify(ScroRecDirector, CCDirector) {
    void drawScene() {
        CCDirector::drawScene();
        recordTick();
    }
};
#endif
