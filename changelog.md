## v2.0.0
### Interface
- Replaced the old sidebar/sub-tab menu with one horizontal row containing the eight source-backed categories.
- Global now displays all registered options without category headings; each other tab filters to its own options.
- Added responsive multi-column layouts, draggable title bar and a movable floating orb for touch devices.
- Added linked sliders and numeric entry fields for every registered step setting. Slider, numeric entry, saved value and live callback all use the same option value.
- Desktop defaults to a light violet palette; mobile defaults to charcoal with violet accents. Light/Dark overrides are available.
- Uses the chat sans-serif font throughout.
### Macro and practice
- Added pause/resume/stop controls to macro recording. The menu stays open while recording so Pause is reachable.
- Starting or stopping macro capture no longer restarts the level.
- Practice checkpoint data remains owned by the game; recording rewinds only trim the macro's future input branch.
- Playback seeks to the current level progress when started.
- Removed recording/executing status labels from the menu.
### Compatibility
- Targets Geometry Dash 2.2091 and Geode 6.0.0.
- Build matrix includes Windows, macOS, iOS, Android 32-bit and Android 64-bit.
- Screen recording remains Android-only.

## v1.5.0
### Fixed
- Buttons no longer disappear after switching tabs.
- Jump Hack raises the on-ground flag when the jump input is handled, rather than after the game has already read the tap.
- Shared menu sizing, larger mobile controls and stable page navigation.
- No Particles no longer re-enables particles hidden by the game.
### Improved
- Safe Mode can be switched freely; cheat-like options still mark runs as test activity.
- Low Detail Mode 2 only changes objects near the screen edge and restores objects on restart or disable.
