#include "Hacks/FrameStep.hpp"
#include "Utils/Settings.hpp"

static auto& freeze = scro::toggle("Global", "freeze", "Freeze Player", "Pauses the physics. Menus keep working.", false, 30);
static auto& fstep = scro::toggle("Global", "framestep", "Frame Step",
    "Pauses the physics and adds a Step button to the Macro tab.", false, 40);
static bool const sections = ((freeze.section = "General"), (fstep.section = "General"), true);
static bool g_pending = false;

bool scro::stepper::frozen() { return freeze.b || fstep.b; }
bool scro::stepper::consume() { bool p = g_pending; g_pending = false; return p; }
void scro::stepper::request() { g_pending = true; }
