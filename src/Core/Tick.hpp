#pragma once
#include <Geode/Geode.hpp>
#include <vector>
// Per-frame callbacks. Hack files register one with `static scro::tick::Reg r(fn);`
namespace scro::tick {
using Fn = void (*)(PlayLayer*, float dt, bool frozen);
inline std::vector<Fn>& fns() { static std::vector<Fn> v; return v; }
struct Reg { explicit Reg(Fn f) { fns().push_back(f); } };
inline void run(PlayLayer* pl, float dt, bool frozen) { for (auto f : fns()) f(pl, dt, frozen); }
}
