#include "Macro/Bot.hpp"
#include "Utils/Settings.hpp"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

static auto& macroPolicy = scro::action("Global", "macropolicy", "Macro Safety Policy",
    "How runs that used a macro are handled.", [] {
        FLAlertLayer::create("Macro Safety",
            "Macros are for practice, testing and showcases. A run that used a macro is always sent as a test run and should not be presented as a legitimate completion.",
            "OK")->show();
    }, 10);
static auto& macroFolder = scro::action("Global", "macrofolder", "Open Macro Folder",
    "Opens the folder that holds your .scro macro files.", [] {
        (void)utils::file::openFolder(scro::Bot::dir().string());
    }, 10);
static auto& dataFolder = scro::action("Global", "datafolder", "Open Data Folder",
    "Opens the folder with this mod's settings.", [] {
        (void)utils::file::openFolder(Mod::get()->getSaveDir());
    }, 20);
static bool const sections = ((macroPolicy.section = "Recording"), (macroFolder.section = "Misc"),
    (dataFolder.section = "Misc"), true);
