#include "UI/Pages.hpp"

namespace scro::ui {
using namespace style;

void buildTheme(Page& p, Overlay& overlay) {
    Overlay* o = &overlay;
    auto const& t = theme();
    p.rowHeading("Theme");
    for (int i = 0; i < themeCount(); ++i) {
        bool current = i == themeIndex();
        auto const& th = themeAt(i);
        // each row is drawn in the colours of the theme it selects
        float h = rowH();
        p.box(0.f, p.y(), p.width(), h, th.card);
        p.box(0.f, p.y(), kSize.pad, h, th.accent);
        p.text(th.name, current ? th.accent : th.text, kSize.pad * 2.f + 2.f, p.y(), h);
        if (current) p.text("on", th.muted, p.width() - kSize.pad, p.y(), h, 1.f);
        p.tap(0.f, p.y(), p.width(), h, [o, i] { setTheme(i); o->rebuildAll(); });
        p.skip(h + kSize.gap);
    }
    (void)t;
    p.rowHeading("Menu");
    buildOptions(p, overlay, "Theme", false);
}
}
