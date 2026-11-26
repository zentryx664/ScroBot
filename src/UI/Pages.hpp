#pragma once
#include "UI/Overlay.hpp"
#include <string>

namespace scro::ui {
void buildOptions(Page& page, Overlay& overlay, std::string const& category, bool footer = true);
void buildMacro(Page& page, Overlay& overlay);
void buildEditor(Page& page, Overlay& overlay);
void buildTheme(Page& page, Overlay& overlay);
}
