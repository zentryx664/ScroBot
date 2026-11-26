#include "Utils/Settings.hpp"

static auto& autosave = scro::toggle("Global", "autosave", "Auto Save", "Saves your settings as you change them.", true, 20);
static bool const section = (autosave.section = "Misc", true);
