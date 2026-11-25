# ScroBot v2.0.0

ScroBot is a Geometry Dash utility and macro toolkit. The menu is a draggable, horizontal settings panel with a responsive desktop/mobile layout.

## Controls
- Desktop: press **Tab + W** to open or close the menu. The floating control can also be dragged and clicked.
- Touch devices: drag the floating orb to reposition it, then tap it to open or close the menu.
- Drag the menu from its title bar to move the panel. Use Back to close it on touch devices.

## Interface
- Eight source-backed tabs: Global, Player, Level, Visuals, Labels, FPS Boost, Replay and Utility.
- Global lists all registered options without category headings. Other tabs show only their own options.
- Step settings have a draggable slider and a numeric text field with Set, both connected to the same stored value.
- Auto theme uses a light violet desktop palette and a charcoal/violet mobile palette; Light and Dark overrides are available.

## Macro tools
- Record inputs from the current level position without restarting the level.
- Pause and resume recording without losing the current macro.
- Save and load local `.scro` macros.
- Play back a loaded macro as an assisted/test run.
- Inspect inputs, step through them, nudge their frame, delete an input or clear the loaded sequence.
- Practice checkpoints remain managed by Geometry Dash. Recording rewinds only branch the macro input timeline.

## Build targets
- Windows x64
- macOS (Apple Silicon and Intel)
- iOS (arm64)
- Android armv7 and arm64

Target versions: Geometry Dash **2.2091**, Geode **6.0.0**. Screen recording remains Android-only.
