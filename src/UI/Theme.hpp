#pragma once
#include "Utils/Settings.hpp"
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>
using namespace geode::prelude;

namespace scro::theme {
inline ccColor3B accent() {
    static Option* o = &find("accent");
    static ccColor3B const P[] = {{56, 160, 255}, {143, 227, 255}, {143, 123, 255}, {62, 224, 198}};
    return P[std::clamp(static_cast<int>(std::lround(o->f)), 0, 3)];
}
// readable text colour on top of the accent (title bars)
inline ccColor3B onAccent() {
    auto a = accent();
    return (a.r * 299 + a.g * 587 + a.b * 114) / 1000 > 150 ? ccColor3B{10, 10, 10} : ccColor3B{255, 255, 255};
}
// darkened accent for selected sidebar entries
inline ccColor3B accentDim() { auto a = accent(); return {static_cast<GLubyte>(a.r * 0.38f), static_cast<GLubyte>(a.g * 0.38f), static_cast<GLubyte>(a.b * 0.38f)}; }
inline ccColor3B const TEXT = {225, 228, 235};
inline ccColor3B const MUTED = {120, 125, 135};
inline ccColor3B const OFF = {60, 60, 64};
}
