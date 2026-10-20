#include "Bot.hpp"
#include <algorithm>
#include <fstream>

namespace scro {
Bot& Bot::get() { static Bot b; return b; }
std::filesystem::path Bot::dir() {
    auto p = Mod::get()->getSaveDir() / "macros";
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
    return p;
}
bool Bot::save(std::string const& name) {
    if (name.empty() || actions.empty()) return false;
    std::ofstream f(dir() / (name + ".scro"), std::ios::binary);
    if (!f) return false;
    uint32_t n = static_cast<uint32_t>(actions.size());
    f.write("SCR1", 4);
    f.write(reinterpret_cast<char const*>(&n), 4);
    f.write(reinterpret_cast<char const*>(actions.data()), n * sizeof(Action));
    return f.good();
}
bool Bot::load(std::string const& name) {
    std::ifstream f(dir() / (name + ".scro"), std::ios::binary);
    if (!f) return false;
    char magic[4]; uint32_t n = 0;
    f.read(magic, 4); f.read(reinterpret_cast<char*>(&n), 4);
    if (!f || std::string(magic, 4) != "SCR1" || n > 5'000'000) return false;
    std::vector<Action> tmp(n);
    if (n) f.read(reinterpret_cast<char*>(tmp.data()), n * sizeof(Action));
    if (n && !f) return false;
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
