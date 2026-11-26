#include "Bot.hpp"
#include <algorithm>
#include <fstream>
#include <cctype>
#include <limits>
#include <cmath>

namespace scro {
Bot& Bot::get() { static Bot b; return b; }
std::filesystem::path Bot::dir() {
    auto p = std::filesystem::path(Mod::get()->getSaveDir().string()) / "macros";
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
    bool ok = f.good();
    if (ok) this->name = name;
    return ok;
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
    // Reject damaged or hand-edited files before they can affect gameplay.
    int32_t lastFrame = -1;
    for (auto const& a : tmp) {
        if (a.frame < 0 || a.frame < lastFrame || a.down > 1 || a.p1 > 1 || a.hasState > 1) return false;
        lastFrame = a.frame;
        if (a.hasState && (!std::isfinite(a.a.x) || !std::isfinite(a.a.y) || !std::isfinite(a.a.rot) || !std::isfinite(a.a.yv) ||
                           !std::isfinite(a.b.x) || !std::isfinite(a.b.y) || !std::isfinite(a.b.rot) || !std::isfinite(a.b.yv))) return false;
    }
    actions = std::move(tmp);
    index = 0;
    this->name = name;
    return true;
}
bool Bot::remove(std::string const& n) {
    if (!validMacroName(n)) return false;
    std::error_code ec;
    bool ok = std::filesystem::remove(dir() / (n + ".scro"), ec);
    if (ok && name == n) name.clear();
    return ok && !ec;
}
std::vector<MacroInfo> Bot::list() {
    std::vector<MacroInfo> out;
    std::error_code ec;
    for (auto& e : std::filesystem::directory_iterator(dir(), ec)) {
        if (e.path().extension() != ".scro") continue;
        MacroInfo info;
        info.name = e.path().stem().string();
        std::ifstream f(e.path(), std::ios::binary);
        char magic[4]; uint32_t n = 0;
        if (f.read(magic, 4) && f.read(reinterpret_cast<char*>(&n), 4) && std::string(magic, 4) == "SCR1") info.inputs = n;
        out.push_back(std::move(info));
    }
    std::sort(out.begin(), out.end(), [](MacroInfo const& a, MacroInfo const& b) { return a.name < b.name; });
    return out;
}
std::string Bot::suggestName() {
    std::string base = "macro";
    if (auto pl = PlayLayer::get(); pl && pl->m_level) {
        std::string levelName = pl->m_level->m_levelName.c_str();
        std::string clean;
        for (char c : levelName) {
            if (std::isalnum(static_cast<unsigned char>(c))) clean += c;
            if (clean.size() >= 24) break;
        }
        if (!clean.empty()) base = clean;
    }
    for (int i = 1; i < 1000; ++i) {
        auto candidate = fmt::format("{}_{:02}", base, i);
        std::error_code ec;
        if (!std::filesystem::exists(dir() / (candidate + ".scro"), ec)) return candidate;
    }
    return base;
}
int Bot::lastFrame() const { return actions.empty() ? 0 : actions.back().frame; }
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
