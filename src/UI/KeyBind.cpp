#include "UI/Menu.hpp"
#include <Geode/loader/SettingV3.hpp>
using namespace geode::prelude;

// PC: Tab (rebindable in the mod settings) opens / closes the menu.
#ifndef GEODE_IS_MOBILE
$execute {
    listenForKeybindSettingPresses("open-menu", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) scro::ui::toggle();
    });
}
#endif
  
