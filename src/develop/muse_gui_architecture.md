# MusE GUI Architecture Notes

Scope: `main.cpp` → `app.cpp` (`MusE`) → `arrangerview.cpp` (`ArrangerView`) →
`arranger.cpp` (`Arranger`) → `tlist.cpp` / `canvas.cpp` / `pcanvas.cpp` / `view.cpp`
→ `ttoolbar.cpp` / `ttoolbutton.cpp` / `tools.cpp`.

---

## 1. Startup / setup order

### 1.1 `main()` (main.cpp)

1. `MuseApplication app(argc, argv)` — QApplication subclass, overrides `notify()`.
2. Locale / config / command-line parsing.
3. `QApplication::addLibraryPath(...)`, style + theme loaded (`MusEGui::loadTheme`).
4. Splash screen (`QSplashScreen muse_splash`) shown, updated with status text as
   subsystems come up (`showMessage(...)`).
5. Plugin cache scan (CLAP/LV2/VST) — optionally forced rescan.
6. `MusECore::initAudio()` — JACK/ALSA audio backend.
7. `MusEGui::initIcons(...)` — icon/theme resources loaded before any widget exists.
8. `MusECore::initMidiSynth()` (conditional on `MusEGlobal::loadMESS`) — must run
   **before** `MusE` is constructed so "Add Track → Synth" is already populated.
9. **`MusEGlobal::muse = new MusEGui::MusE();`** ← main window constructed here.
10. `app.setMuse(MusEGlobal::muse)`.
11. `MusEGui::init_function_dialogs()` / `retranslate_function_dialogs()`.
12. Audio driver init, realtime priority setup.
13. `MusEGlobal::muse->show()`.
14. Event loop; on exit, `MusEGlobal::muse = nullptr`.

Key point: **icons and MIDI synth info must exist before `MusE::MusE()` runs**,
since the constructor immediately builds menus/actions that reference them.

### 1.2 `MusE::MusE()` (app.cpp, ~line 488)

Order inside the constructor (this is the authoritative sequence — components
are created in dependency order, not declaration order):

1. **Window basics**: icon size, focus policy, window title/icon.
2. **Member init**: all dialog pointers set to `nullptr` (lazily created later,
   see §3).
3. **Core objects**: `MusEGlobal::globalRasterizer`, `MusEGlobal::song` (with
   signals blocked until the end of the constructor), heartbeat/blink/save/
   message-poll `QTimer`s + their `connect()`s.
4. **Dock widgets** (created + hidden by default, shown later via menu actions):
   - `markerDock` → `MarkerView` (`markerView`)
   - `masterListDock` → `LMaster` (`masterList`)
   - `clipListDock` → `ClipListEdit` (`clipListEdit`)
   - `mixer1Dock` → `AudioMixerApp` (`mixer1`) — only if `dockMixerA` configured
   - `mixer2Dock` → `AudioMixerApp` (`mixer2`) — only if `dockMixerB` configured
5. **Global QActionGroups / QActions** (before menus/toolbars, since both consume
   these actions):
   - `MusEGlobal::undoRedo` → undo/redo actions
   - `MusEGlobal::transportAction` → loop/punch-in/punch-out/start/rewind/
     forward/stop/play/record + separator
   - `panicAction`, `metronomeAction`
   - File actions (`fileNewAction`, `fileOpenAction`, `fileSaveAction`, ...)
   - View/Midi/Audio/Automation/Settings/Help actions (all `QAction` members
     of `MusE`, ~80 of them)
   - Menu-only `QMenu` objects created here too where they need early
     population: `openRecent`, `follow` (with its own `QActionGroup`).
6. **Toolbars** (`addToolBar(...)`), each populated from the actions created
   in step 5:
   - `tools` ("File Buttons"): new/new-from-template/open/save + What's This
   - `undo_tools` ("Undo/Redo")
   - `panic_toolbar` ("Panic")
   - `metronome_toolbar` ("Metronome")
   - `cpuLoadToolbar` (`CpuToolbar`, hidden by default — info now lives in the
     status bar)
   - `songpos_tb` ("Timeline") — hosts `SongPosToolbarWidget`, moved to
     `Qt::BottomToolBarArea`
   - `transportToolbar` ("Transport") — forced `Qt::LeftToRight` layout
     (time-flow widgets must not mirror under RTL locales)
   - `recToolbar` (`RecToolbar`), `syncToolbar` (`SyncToolbar`)
   - `addToolBarBreak()`
   - `tempo_tb` (`TempoToolbar`), `sig_tb` (`SigToolbar`), `posToolbar`
     (`PosToolbar`)
   - Toolbars are registered into `requiredToolbars` / `optionalToolbars`
     vectors (used later for per-`TopWin` toolbar-sharing logic).
7. **Menu bar** (`menuBar()->addMenu(...)`, always via a `new QMenu(this)`
   first — **never** `menuBar()->addMenu(QString&)`, so menu ownership stays
   with `MusE` and not the menu bar; this matters for `TopWin` menu-sharing):
   `menu_file`, `menuView`, `menu_functions` ("&Midi"), `menu_audio`,
   `menuWindows`, `menuSettings`, `menuHelp`. Each is pushed onto
   `leadingMenus` / `trailingMenus`.
8. **Central widget**: `mdiArea` (`MuseMdiArea`, tabbed view, tabs at bottom,
   south-position `QTabWidget`) via `setCentralWidget(mdiArea)`.
9. **Arranger**: `arrangerView = new MusEGui::ArrangerView(this)` — this is
   the first (and permanent) MDI sub-window/top-level window; pushed onto
   `toplevels`. `_arranger = arrangerView->getArranger()` caches the inner
   `Arranger` widget pointer.
10. Recent-projects list loaded from `<configPath>/projects`.
11. `transport = new MusEGui::Transport(this, "transport")` (separate
    top-level transport panel), `bigtime = nullptr` (lazy).
12. `MusEGlobal::song->blockSignals(false)` — song signals re-enabled only now
    that the whole GUI exists and can react to them.
13. Geometry restore or `centerAndResize()`, `setAndAdjustFonts()`,
    `MusEGlobal::song->update()`, `updateWindowMenu()`.

**Why this order matters**: actions must exist before both menus and toolbars
reference them; toolbars/menus must exist before `ArrangerView` is constructed,
because `ArrangerView`'s own toolbars use `MusE::setCurrentMenuSharingTopwin()`
object-name matching against `MusE`'s toolbars; `song`'s signals stay blocked
throughout construction to avoid partially-built widgets reacting to update
signals.

### 1.3 `ArrangerView::ArrangerView()` (arrangerview.cpp, line 87)

A `TopWin` subclass (`TopWin::ARRANGER`). Setup order:

1. `visTracks = new VisibleTracks(this)`.
2. `arranger = new Arranger(this, "arranger")`; `setCentralWidget(arranger)`.
3. Toolbars, in order: `editTools` (`EditToolBar`, object name
   `"arrangerTools"`), `automationModeToolBar`, `visTracks` (added as a
   toolbar too), `partColorToolBar`.
4. Cross-widget `connect()`s wiring `editTools` ↔ `arranger`,
   `partColorToolBar` ↔ `arranger->getCanvas()`, config-change propagation.
5. A large block of `QAction`s for Edit/Select/Score/Editor/Structure menus
   (`editDeleteAction`, `select` menu, `scoreSubmenu`, `editorNewSubmenu`,
   etc.).

### 1.4 `Arranger::Arranger()` (arranger.cpp, line 243)

The actual split-view widget embedded as `ArrangerView`'s central widget:

1. Rasterizer/grid model setup (`_rasterizerModel`, `_raster`).
2. **Arranger toolbar** (`parent->addToolBar(tr("Arranger"))`, object name
   `"ArrangerToolbar"`): cursor position label, grid-on button, raster combo,
   song-length spinbox, pitch/tempo spinboxes, tempo-preset buttons.
3. **Layout**: root `QVBoxLayout` (`box`) containing a horizontal line, then a
   `Splitter` (`split`) with three panes:
   - `trackInfoWidget` (`TrackInfoWidget`) — per-track control strip
   - `tracklistScroll` (`QScrollArea`) wrapping `tracklist` (`QWidget`), which
     itself contains `header` (`Header`) + `list` (`TList`) stacked in a
     `QVBoxLayout` (`tlistLayout`)
   - `editor` (`QWidget`) — the timeline/canvas area
4. `genTrackInfo(trackInfoWidget)` populates the track-info strip.
5. Editor internals inside `editor`, arranged via `QGridLayout` (`egrid`):
   - `time` (`MTScale`) — ruler, row 0
   - horizontal separator line, row 1
   - `canvas` (`PartCanvas`) — row 2, col 0
   - `vscroll` (`QScrollBar`, vertical) — row 2, col 1
   - `bottomHLayout` containing `hscroll` (`ScrollScale`) — row 3
6. Wiring: `header ↔ list` (resize/redraw), `canvas ↔ list` (track selection,
   mute/solo/volume/pan forwarding, keypress redirection), `vscroll`/`hscroll`
   ↔ `canvas`/`list`/`time` (position and zoom sync).

Tracklist layout (from the source comment, arranger.cpp ~line 431):

```
         0         1         2
   +-----------+--------+---------+
   | Trackinfo | scroll | Header  | 0
   |           | bar    +---------+
   |           |        | TList   | 1
   +-----------+--------+---------+
   |             hline            | 2
   +-----+------------------------+
   | ib  |                        | 3
   +-----+------------------------+
```

---

## 2. Component name reference

| Owner            | Member name         | Type                    | Role |
|------------------|----------------------|--------------------------|------|
| `MusE`           | `mdiArea`            | `MuseMdiArea`            | central widget, tabbed MDI host |
| `MusE`           | `arrangerView`       | `MusEGui::ArrangerView`  | permanent arranger sub-window |
| `MusE`           | `transport`          | `MusEGui::Transport`     | transport panel (top-level) |
| `MusE`           | `bigtime`            | `MusEGui::BigTime` (lazy)| large time display |
| `MusE`           | `markerDock` / `markerView` | `QDockWidget` / `MarkerView` | marker list |
| `MusE`           | `masterListDock` / `masterList` | `QDockWidget` / `LMaster` | master track list |
| `MusE`           | `clipListDock` / `clipListEdit` | `QDockWidget` / `ClipListEdit` | clip list |
| `MusE`           | `mixer1` / `mixer2`  | `AudioMixerApp`          | audio mixers (optionally docked) |
| `MusE`           | `tools`              | `QToolBar`               | file buttons |
| `MusE`           | `cpuLoadToolbar`     | `CpuToolbar`             | CPU/xrun display (hidden by default) |
| `MusE`           | `menu_file`, `menuView`, `menu_functions`, `menu_audio`, `menuWindows`, `menuSettings`, `menuHelp` | `QMenu` | top-level menus |
| `ArrangerView`   | `arranger`           | `Arranger`               | central widget |
| `ArrangerView`   | `visTracks`          | `VisibleTracks`          | track-visibility toolbar |
| `ArrangerView`   | `editTools`          | `EditToolBar`            | tool-mode selector (pointer/pencil/eraser/...) |
| `ArrangerView`   | `automationModeToolBar` | `AutomationModeToolBar` | automation draw mode |
| `ArrangerView`   | `partColorToolBar`   | `PartColorToolbar`       | part color picker |
| `Arranger`       | `split`              | `Splitter`               | horizontal 3-pane splitter |
| `Arranger`       | `trackInfoWidget`    | `TrackInfoWidget`        | per-track strip (mute/solo/volume/...) |
| `Arranger`       | `tracklistScroll` / `tracklist` | `QScrollArea` / `QWidget` | scrollable track list container |
| `Arranger`       | `header`             | `Header`                 | column header above `TList` |
| `Arranger`       | `list`               | `TList`                  | track list rows |
| `Arranger`       | `editor`             | `QWidget`                | timeline/canvas container |
| `Arranger`       | `time`               | `MTScale`                | bar/beat ruler |
| `Arranger`       | `canvas`             | `PartCanvas`             | part display/edit surface |
| `Arranger`       | `vscroll` / `hscroll`| `QScrollBar` / `ScrollScale` | scroll + zoom controls |
| `EditToolBar`    | `toolList` / `toolShortcuts` | static `QVector<ToolB>` / `QMap<int,int>` | tool icon/tooltip/shortcut table |
| `EditToolBar`    | `actionGroup`        | `QActionGroup`           | exclusive tool selection |

### Class hierarchy (view → canvas)

- `View` (view.cpp) — base scrollable/zoomable widget; owns raw mouse/key
  event handlers (`mousePressEvent`, `mouseMoveEvent`, `keyPressEvent`).
- `Canvas` (canvas.cpp) — extends `View`; introduces the `CItem`-based
  selection/drag model and the `viewMouse*Event` virtual dispatch
  (`viewMousePressEvent`, `viewMouseMoveEvent`, `viewMouseReleaseEvent`).
- `PartCanvas` (pcanvas.cpp) — extends `Canvas`; implements part-specific
  behavior (drag & drop of song/MIDI files, part rendering, tool logic).
- `TList` (tlist.cpp) — sibling widget, not derived from `Canvas`/`View`; own
  `paintEvent`/`mousePressEvent`/`mouseMoveEvent`/`mouseReleaseEvent`/
  `keyPressEvent`/`wheelEvent` implementation for the track-list rows.

---

## 3. Event setup and delegation

### 3.1 Signal/slot wiring pattern

Almost all cross-component communication is Qt signal/slot, connected once at
construction time in the owning parent (`MusE`, `ArrangerView`, `Arranger`).
Two connection styles are used side by side:

- Old-style string-based: `connect(sender, SIGNAL(x()), receiver, SLOT(y()))`
  — dominant style, kept for consistency with the surrounding legacy code.
- Modern function-pointer / lambda style: e.g.
  `connect(mixer1Dock, &QDockWidget::topLevelChanged, [this](bool b){ mixer1DockTopLevelChanged(b); });`
  used for a handful of newer additions (dock float/embed handling, automation
  mode toolbar, `configChanged` meta-connection stored as
  `_configChangedEditToolsMetaConn` so it can be explicitly disconnected later).

### 3.2 Delegation chains

- **Tool selection**: `EditToolBar::actionGroup` (`QActionGroup`, exclusive)
  → `triggered(QAction*)` → `EditToolBar::toolChanged(QAction*)` (private
  slot) → re-emits `toolChanged(int)` (public signal, same name — see note
  below) → consumed by `Arranger::setTool(int)`. The reverse path
  (`Arranger::setUsedTool(int)` / `Canvas::setUsedTool(int)`) calls back into
  `EditToolBar::set(int)` to keep the toolbar's checked action in sync when
  the tool changes programmatically (e.g. keyboard shortcut, escape key).
- **Track selection / list ↔ canvas**: `PartCanvas` emits
  `trackChanged(Track*)`, `selectTrackAbove()/Below()`, `muteSelectedTracks()`,
  `soloSelectedTracks()`, `volumeSelectedTracks(int)`,
  `panSelectedTracks(int)` — all consumed by `TList` slots
  (`selectTrack`, `selectTrackAbove/Below`, `muteSelectedTracksSlot`, ...).
  `TList` emits `keyPressExt(QKeyEvent*)` back to `canvas->redirKeypress(...)`
  so keyboard focus can stay on `TList` while key events still drive canvas
  navigation.
- **Scroll/zoom sync**: `hscroll` (`ScrollScale`) emits `scrollChanged(int)` /
  `scaleChanged(int)`, fanned out to `canvas`, `time` (`MTScale`) via
  `setXPos`/`setXMag`. `vscroll` emits `valueChanged(int)` fanned out to
  `canvas` and `list` via `setYPos`.
- **Global config changes**: `MusEGlobal::muse` emits `configChanged()`
  (fired from `MusE::updateConfiguration()` — not shown in these files) and
  is picked up by `partColorToolBar->configChanged()`,
  `arranger->configChanged()`, and `editTools->configChanged()`. This is the
  central "settings changed, please re-read config" broadcast — new widgets
  that need to react to theme/shortcut/appearance changes should connect
  here rather than polling.
- **Raw input event dispatch** (`View` → `Canvas` → `PartCanvas`):
  `View` receives the real Qt events (`mousePressEvent`, etc.) and forwards
  into the `viewMouse*Event` virtual functions that `Canvas` overrides/
  extends; `PartCanvas` further overrides behavior for part-specific hit
  testing, drag creation, and `dragEnterEvent`/drop handling (song/MIDI file
  drops, forwarded up as `dropSongFile(QString)` / `dropMidiFile(QString)`
  signals consumed by `MusE::loadProjectFile` / `MusE::importMidi`).
- **Toolbar action → global state**: `ttoolbar.cpp`'s free function
  `syncChanged(bool flag)` is a plain (non-member) event handler invoked when
  the transport's sync-to-external-clock state toggles; it just disables/
  enables the play/stop/rewind/forward/start actions in bulk — a simple
  example of "delegation without a class", kept as a free function rather
  than a slot.

### 3.3 Note on `EditToolBar::toolChanged`

`EditToolBar` has both a private slot `void toolChanged(QAction*)` and a
public signal `void toolChanged(int)` sharing the same name (overload
resolved by signature). This is legal Qt/C++ but easy to misread — when
grepping for `toolChanged`, check the parameter type to know whether you're
looking at the signal or the slot.

### 3.4 Menu-sharing / toolbar-name matching

Toolbar object names (`"Transport tool"`, `"arrangerTools"`, etc.) are
intentionally kept consistent across `MusE` and every `TopWin` subclass
(`ArrangerView`, and others not shown here). `MusE::setCurrentMenuSharingTopwin()`
uses these names to find-and-replace (`insertToolBar`) an existing toolbar
rather than appending a duplicate, when a `TopWin` becomes the active
top-level window. Any new toolbar added to a `TopWin` subclass must use a
name that either matches an equivalent `MusE` toolbar (for shared behavior)
or is unique (to avoid unintended replacement).

---

## 4. Additional files that would help

To fully trace initialization and event delegation without gaps, these would
be useful if available:

- **`topwin.cpp` / `topwin.h`** — `TopWin` base class; `setCurrentMenuSharingTopwin()`
  and toolbar-sharing logic are referenced constantly but not defined in the
  uploaded files.
- **`header.cpp`** — `Header` class (`initTracklistHeader()` is called in
  `Arranger::Arranger` but its implementation isn't in the uploaded set).
- **`app.h` / `arranger.h` / `arrangerview.h`** — header files would confirm
  member types/visibility without relying on constructor-body inference
  (useful to double check e.g. `_arranger`, `toplevels`, `requiredToolbars`
  types).
- **`ctrlcanvas.cpp` or automation-related canvas file** — `AutomationModeToolBar`
  is wired here but its own signal/slot internals aren't visible.
- **`song.cpp`** — `MusEGlobal::song` drives a large share of the update/dirty
  signal chain (`sigDirty()`, `beat()`, `update()`); seeing its emit sites
  would complete the delegation picture in §3.2.

Let me know if you'd like me to fold any of these in once you upload them, or
if the current scope is enough for now.
