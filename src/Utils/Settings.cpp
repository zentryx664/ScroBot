#include "Settings.hpp"
#include <algorithm>
#include <cmath>
#include <deque>
using namespace geode::prelude;

namespace scro {
char const* const kCategories[] = {"Global", "Player", "Level", "Visuals", "Labels", "FPS Boost", "Replay", "Utility", "Settings"};

static std::deque<Option>& all() { static std::deque<Option> d; return d; }   // stable references

static Option& add(Option o) { all().push_back(std::move(o)); return all().back(); }

Option& toggle(char const* cat, char const* id, char const* name, char const* desc, bool def, int order) {
    Option o; o.cat = cat; o.id = id; o.name = name; o.desc = desc; o.kind = Kind::Toggle; o.b = def; o.defB = def; o.order = order;
    return add(std::move(o));
}
Option& step(char const* cat, char const* id, char const* name, char const* desc, float def,
             float mn, float mx, float st, char const* unit, int order) {
    Option o; o.cat = cat; o.id = id; o.name = name; o.desc = desc; o.kind = Kind::Step;
    o.f = def; o.defF = def; o.mn = mn; o.mx = mx; o.st = st; o.unit = unit; o.order = order;
    return add(std::move(o));
}
Option& action(char const* cat, char const* id, char const* name, char const* desc, std::function<void()> fn, int order) {
    Option o; o.cat = cat; o.id = id; o.name = name; o.desc = desc; o.kind = Kind::Action; o.action = std::move(fn); o.order = order;
    return add(std::move(o));
}
Option& find(char const* id) {
    for (auto& o : all()) if (o.id == id) return o;
    static Option none; return none;
}
std::vector<Option*> inCategory(std::string const& cat) {
    std::vector<Option*> v;
    for (auto& o : all()) if (o.cat == cat) v.push_back(&o);
    std::stable_sort(v.begin(), v.end(), [](Option* a, Option* b) { return a->order < b->order; });
    return v;
}
std::vector<Option*> allOptions() {
    std::vector<Option*> v;
    v.reserve(all().size());
    for (auto& o : all()) v.push_back(&o);
    std::stable_sort(v.begin(), v.end(), [](Option* a, Option* b) {
        if (a->cat != b->cat) return a->cat < b->cat;
        if (a->order != b->order) return a->order < b->order;
        return a->name < b->name;
    });
    return v;
}
std::string Option::text() const {
    if (!names.empty()) return names[std::clamp(static_cast<int>(std::lround(f)), 0, static_cast<int>(names.size()) - 1)];
    if (unit == "x") return fmt::format("{:.2f}x", f);
    return fmt::format("{:.0f}{}", f, unit);
}
void load() {
    auto m = Mod::get();
    for (auto& o : all()) {
        if (o.kind == Kind::Toggle) o.b = m->getSavedValue<bool>(o.id, o.b);
        else if (o.kind == Kind::Step) o.f = m->getSavedValue<float>(o.id, o.f);
    }
}
void save() {
    auto m = Mod::get();
    for (auto& o : all()) {
        if (o.kind == Kind::Toggle) m->setSavedValue(o.id, o.b);
        else if (o.kind == Kind::Step) m->setSavedValue(o.id, o.f);
    }
}
void changed() {
    static Option* autosave = &find("autosave");
    if (!autosave) return;
    if (autosave->b) save();
    else Mod::get()->setSavedValue("autosave", false);
}
void reset(std::string const& cat) {
    for (auto& o : all()) {
        if (o.cat != cat) continue;
        if (o.kind == Kind::Toggle) o.b = o.defB;
        else if (o.kind == Kind::Step) o.f = o.defF;
        else continue;
        if (o.onChange) o.onChange();
    }
}
}
