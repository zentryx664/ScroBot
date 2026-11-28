## v1.0.0

First public release of ScroBot: a floating menu button that opens a panel of tools for Geometry Dash on Android and iOS (and desktop).

### Menu and interface
- **Floating button** with the ScroBot lightning-bolt logo. Tap it to open or close the menu, drag it anywhere, and its position is remembered. It pulses when tapped and the menu unfolds out of it with a smooth animation.
- **Draggable tabs** (Global, Player, Level, Visuals, Labels, FPS Boost, Macro, Editor, Theme). Tap a tab's title to collapse it. A tab you drop on top of another slides to the nearest free spot, so tabs never end up stacked.
- **Remembered layout:** tab positions are saved the moment you let go and again when the app goes to the background, so the layout does not jump when you leave Geode and come back.
- **Themes:** Violet, Midnight, Mono, Paper, Ocean, Forest and Rose.
- **Gradient:** choose a gradient for the tab title bars (Aurora, Sunset, Ocean, Forest, Candy, Fire, or Off).
- **Menu Size, Ball Size and Resize:** scale the whole menu, the button, or the tabs, rows and buttons separately.
- **Reset Layout** and **Reset Ball** put everything back where it started.
- **Faster menu:** every tab draws its backgrounds in a single batch, only the tab you tapped is rebuilt, and rows outside the scroll area are not drawn.
- **Desktop:** hold Tab and press W to open or close the menu.

### Global
- **Safe Mode:** completions count as test runs. Cheat-like options (No Clip, Jump Hack, Speedhack, Boost Frames, macros) always send a test run, whatever this says.
- **No Clip**, **Speedhack**, **Freeze Player** and **Frame Step** (adds a Step button to the Macro tab).
- **Fix Click** (Misc): blocks double clicks; the same press or release can only happen once per tick.
- **Screen Record** with game sound into your gallery (Movies/ScroBot), with **Record FPS** and **Record Quality**.
- **Macro Safety Policy**, **Auto Save**, **Open Macro Folder** and **Open Data Folder**.

### Player
- **Jump Hack** (jump in mid-air; completions count as test runs).
- **No Rotation**.
- **Solid Wave Trail**.

### Level
- **Show Level Info** (name, author and ID at the start of a level).
- **Auto Practice** (start every level in practice mode).
- **Practice Music Sync** (resync the music on practice respawn).

### Visuals
- **Hitboxes.**
- **Hide UI.**
- **No Glow**, **No Particles**, **No Death Effect.**
- **Low Detail Mode 2:** deletes blocks and decoration you have already passed so the game draws less. Triggers and gameplay objects are never touched, and everything comes back on death, restart, checkpoint respawn, when you turn it off, or when you leave the level. It turns itself off in platformer levels.
- **Smooth:** smooth texture filtering at full quality, with no background or ground textures, no glow and no particles.

### Labels
- **FPS**, **CPS**, **Clock**, **Run Time** and **Frame** labels, with adjustable **Label Opacity**.
- **Frame Counter:** reads the frame window of every click, like a macro that also measures its timings. It counts clicks in coloured buckets (9-12, 7-8, 5-6, 4, 3, 2, 0-1 frames), marks each click with a square, and plays a sound where fewer frames means a lower pitch and more frames a higher pitch. With a macro playing it measures the run; with it off, macros play normally. Also available as a toggle in the Macro tab.
- **Counter Sound**, **Counter Squares** and **Reset Frame Counter.**

### FPS Boost
- **FPS Boost** with **Target FPS** presets from 60 up to 1000. Once chosen, the frame rate is locked: no drops below it, no going above it. It is re-checked every second, menus included.
- **Boost Frames**, **Click Precision** (applies every tap at its exact moment between physics steps) and **Zero-Delay CBF.**
- **Frame-Time Diagnostics** in the level HUD.

### Macros
- **Record and play back** inputs with player position correction, then **save, load and delete** macros.
- **Macro Editor:** select inputs, edit selected inputs or the whole macro, use the edit history, and browse inputs page by page.
- A run that was helped by a macro is never counted as a clean completion.