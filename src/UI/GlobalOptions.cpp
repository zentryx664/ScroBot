#include "Macro/Bot.hpp"
#include "Utils/Settings.hpp"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

// Small, useful entries for the new Global sub-sections. These keep the Global
// page useful without duplicating the gameplay toggles in their existing tabs.
static auto& macroPolicy = scro::action("Global", "macropolicy", "Macro Safety Policy",
    "Explains how Scro marks macro-assisted runs.", [] {
        FLAlertLayer::create("Scro Macro Safety",
            "Macros are intended for practice, testing and level showcases. Scro marks macro-assisted runs as test/showcase runs where supported, and they should not be presented as legitimate completions.",
            "Got it")->show();
    }, 10);
static auto& macroFolder = scro::action("Global", "macrofolder", "Open Macro Folder",
    "Open the local folder used to exchange .scro files manually.", [] {
        (void)utils::file::openFolder(scro::Bot::dir());
    }, 10);
static auto& dataFolder = scro::action("Global", "datafolder", "Open Scro Data Folder",
    "Open Scro's local settings and data directory.", [] {
        (void)utils::file::openFolder(Mod::get()->getSaveDir());
    }, 20);
static bool const sectionSetup = ((macroPolicy.section = "Recording"), (macroFolder.section = "Misc"),
    (dataFolder.section = "Misc"), true);
