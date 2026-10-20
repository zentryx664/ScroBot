#include "Utils/Settings.hpp"
using namespace geode::prelude;

static auto& opacity = scro::step("Settings", "menuopacity", "Menu Opacity", "", 94.f, 40.f, 100.f, 5.f, "%", 10);
static auto& compact = scro::toggle("Settings", "compact", "Compact Rows", "Reduce row height on mobile.", false, 20);
static scro::Option& accent = scro::step("Settings", "accent", "Accent", "Applies next time a menu opens.", 0.f, 0.f, 3.f, 1.f, "", 30);
static bool _names = (accent.names = {"Azure", "Ice", "Violet", "Teal"}, true);
static auto& folder = scro::action("Settings", "cfgfolder", "Config Folder", "Open the settings folder.",
    [] { (void)utils::file::openFolder(Mod::get()->getSaveDir()); }, 40);
