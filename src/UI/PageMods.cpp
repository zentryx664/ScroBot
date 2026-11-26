#include "Notifications/Notify.hpp"
#include "UI/Pages.hpp"
#include "Utils/Settings.hpp"
#include <algorithm>
#include <cmath>

namespace scro::ui {
using namespace style;

namespace {
void optionRow(Page& p, Overlay* o, scro::Option* opt) {
    switch (opt->kind) {
        case scro::Kind::Toggle:
            p.rowToggle(opt->name, opt->b, [opt, o] {
                opt->b = !opt->b;
                if (opt->onChange) opt->onChange();
                scro::changed();
                o->setInfo(opt->desc);
                o->refresh();
            });
            break;
        case scro::Kind::Step: {
            auto bump = [opt, o](int dir) {
                float v = opt->f + static_cast<float>(dir) * opt->st;
                v = std::round(std::clamp(v, opt->mn, opt->mx) * 1000.f) / 1000.f;
                if (v != opt->f) {
                    opt->f = v;
                    if (opt->onChange) opt->onChange();
                    scro::changed();
                }
                o->setInfo(opt->desc);
                o->refresh();
            };
            p.rowStep(opt->name, opt->text(), [bump] { bump(-1); }, [bump] { bump(1); });
            break;
        }
        case scro::Kind::Action:
            p.rowAction(opt->name, Btn::Normal, [opt, o] {
                o->setInfo(opt->desc);
                if (opt->action) opt->action();
            });
            break;
    }
}
}

void buildOptions(Page& p, Overlay& overlay, std::string const& category, bool footer) {
    Overlay* o = &overlay;
    auto options = scro::inCategory(category);
    if (options.empty()) p.rowLabel("Nothing here on this device", theme().muted);

    std::vector<std::string> sections;
    for (auto* opt : options)
        if (std::find(sections.begin(), sections.end(), opt->section) == sections.end()) sections.push_back(opt->section);
    for (auto const& section : sections) {
        if (sections.size() > 1) p.rowHeading(section);
        for (auto* opt : options) if (opt->section == section) optionRow(p, o, opt);
    }
    if (!footer || options.empty()) return;
    std::string cat = category;
    p.rowCells({
        {"Save", Btn::Quiet, [] { scro::save(); scro::notify("Settings saved"); }},
        {"Reset", Btn::Quiet, [o, cat] { scro::reset(cat); scro::changed(); o->refresh(); }},
    });
}
}
