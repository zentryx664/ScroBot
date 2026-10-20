#pragma once
#include <Geode/Geode.hpp>
namespace scro {
inline void notify(std::string const& text, bool ok = true) {
    geode::Notification::create(text, ok ? geode::NotificationIcon::Success : geode::NotificationIcon::Error)->show();
}
}
