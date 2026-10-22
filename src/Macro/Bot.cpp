#include "Bot.hpp"
#include <algorithm>
#include <fstream>
#include <cctype>
#include <limits>
#include <cmath>

namespace scro {
Bot& Bot::get() { static Bot b; return b; }
std::filesystem::path Bot::dir() {
    auto p = Mod::get()->getSaveDir() / "macros";
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
    return p;
}
namespace {
bool validMacroName(std::string const& name) {
    if (name.empty() || name.size() > 64) return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-';
    });
}
}
bool Bot::save(std::string const& name) {
    if (!validMacroName(name) || actions.empty() || actions.size() > 1'000'000) return false;
    std::ofstream f(dir() / (name + ".scro"), std::ios::binary);
    if (!f) return false;
    uint32_t n = static_cast<uint32_t>(actions.size());
    f.write("SCR1", 4);
    f.write(reinterpret_cast<char const*>(&n), 4);
    f.write(reinterpret_cast<char const*>(actions.data()), n * sizeof(Action));
    return f.good();
}
bool Bot::load(std::string const& name) {
    if (!validMacroName(name)) return false;
    auto path = dir() / (name + ".scro");
    std::error_code ec;
    auto bytes = std::filesystem::file_size(path, ec);
    if (ec || bytes < 8 || bytes > (8ull + 1'000'000ull * sizeof(Action))) return false;
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    char magic[4]; uint32_t n = 0;
    f.read(magic, 4); f.read(reinterpret_cast<char*>(&n), 4);
    if (!f || std::string(magic, 4) != "SCR1" || n > 1'000'000 || bytes != 8ull + static_cast<uint64_t>(n) * sizeof(Action)) return false;
    std::vector<Action> tmp(n);
    if (n) f.read(reinterpret_cast<char*>(tmp.data()), static_cast<std::streamsize>(n * sizeof(Action)));
    if (n && !f) return false;
    // Reject corrupt/untrusted workshop files before they can affect gameplay.
    int32_t lastFrame = -1;
    for (auto const& a : tmp) {
        if (a.frame < 0 || a.frame < lastFrame || a.down > 1 || a.p1 > 1 || a.hasState > 1) return false;
        lastFrame = a.frame;
        if (a.hasState && (!std::isfinite(a.a.x) || !std::isfinite(a.a.y) || !std::isfinite(a.a.rot) || !std::isfinite(a.a.yv) ||
                           !std::isfinite(a.b.x) || !std::isfinite(a.b.y) || !std::isfinite(a.b.rot) || !std::isfinite(a.b.yv))) return false;
    }
    actions = std::move(tmp); index = 0;
    return true;
}
void Bot::seek(int frame) {
    index = std::lower_bound(actions.begin(), actions.end(), frame,
        [](Action const& a, int fr) { return a.frame < fr; }) - actions.begin();
}
PState grab(PlayerObject* p) {
    if (!p) return {0, 0, 0, 0};
    return {p->getPositionX(), p->getPositionY(), p->getRotation(), static_cast<float>(p->m_yVelocity)};
}
void apply(PlayerObject* p, PState const& s) {
    if (!p) return;
    p->setPosition({s.x, s.y});
    p->setRotation(s.rot);
    p->m_yVelocity = s.yv;
}
}
