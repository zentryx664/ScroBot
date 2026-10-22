#pragma once
#include <Geode/Geode.hpp>
#include <functional>
#include <string>
#include <vector>

namespace scro {
enum class Kind { Toggle, Step, Action };

struct Option {
    std::string cat, id, name, desc, unit;
    std::string section = "General";         // sub-heading inside a category
    Kind kind = Kind::Toggle;
    int order = 0;
    bool b = false, defB = false;
    float f = 0, defF = 0, mn = 0, mx = 1, st = 1;
    std::vector<std::string> names;          // optional labels for Step values
    std::function<void()> action;            // Kind::Action
    std::function<void()> onChange;          // called after the UI changes the value
    std::string text() const;
};

Option& toggle(char const* cat, char const* id, char const* name, char const* desc, bool def, int order);
Option& step(char const* cat, char const* id, char const* name, char const* desc, float def,
             float mn, float mx, float st, char const* unit, int order);
Option& action(char const* cat, char const* id, char const* name, char const* desc,
               std::function<void()> fn, int order);
Option& find(char const* id);                              // cache the result at the call site
std::vector<Option*> inCategory(std::string const& cat);   // sorted by `order`
std::vector<Option*> allOptions();                            // all options, stable references

extern char const* const kCategories[];
constexpr int kNumCategories = 9;

void load();
void save();                         // always writes to disk
void changed();                      // writes only if Auto Save is on
void reset(std::string const& cat);  // restore a category's defaults
}
