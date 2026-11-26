#include "Utils/Settings.hpp"
#include "Hacks/FpsBoost.hpp"
$on_mod(Loaded) {
    scro::fps::initOptions();
    scro::load();
}
