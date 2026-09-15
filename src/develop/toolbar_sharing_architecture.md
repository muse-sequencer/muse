# MusE Toolbar Sharing: Shared vs. Non-Shared Toolbars

## The problem this solves

`MusE` (the main window) and every editor window (`ArrangerView`, `MidiEditor`
and its subclasses like Pianoroll/DrumEdit/WaveEdit/ScoreEdit — all `TopWin`
subclasses) each want to show the same toolbars: Transport, Tempo, Position,
etc. Only one editor is ever "active" at a time (tabbed/MDI mode), so only one
copy of each toolbar should ever be visible — but naively, every `TopWin`
subclass building its own full copy leads to N independently-constructed
instances (one per editor type ever opened), which is wasteful and — if the
swap-in mechanism has any name mismatch — visibly duplicates toolbars.

Two categories of toolbar exist, handled differently:

## 1. Shared toolbars (single instance, reused everywhere)

**Toolbars:** Undo/Redo, Panic, Metronome, Timeline, Transport, Recording,
Sync, Position, Tempo, Signature.

**Owner / constructor:** `MusE::MusE()` (`app.cpp`). Built once, added to
`optionalToolbars` (and looked up later by name via `MusE::sharedOptionalToolBar()`,
declared in `app.h`).

**Consumer:** `TopWin::TopWin()` (`cobject.cpp`, the base class of every
editor window). Instead of constructing its own copy, it calls
`muse->sharedOptionalToolBar("Transport tool")` etc. and pushes the returned
pointer directly into its own `_toolbars` bookkeeping list (bypassing
`TopWin::addToolBar()`'s hide/show logic — a borrowed toolbar is never shown
or hidden by the borrower).

**Why this works:** `MusE::MusE()` always finishes building its own toolbars
*before* any `TopWin` is constructed (the first one, `ArrangerView`, is built
near the end of `MusE::MusE()`; every other editor type is opened later still,
long after `MusE` fully exists). So `sharedOptionalToolBar()` can always find
what it's looking for.

**Signal routing:** Position/Tempo/Signature emit `returnPressed()` /
`escapePressed()` (used to return focus to the canvas after editing a value).
Since there is only one instance now, it can't be wired to a fixed target
(`this` TopWin) at construction time — instead `MusE::MusE()` wires it once,
dynamically, to whichever editor is currently focused:

```cpp
connect(posToolbar, &PosToolbar::returnPressed,
        [this]() { if(activeTopWin) activeTopWin->focusCanvas(); });
```

`MusE::activeTopWin` is kept up to date by the existing focus-tracking
mechanism (`ACTIVE TOPWIN CHANGED`), and `TopWin::focusCanvas()` is `virtual`,
so this dispatches correctly to whichever concrete editor is active. Exposed
via `MusE::getActiveTopWin()` for anything else that needs it.

**Ownership / destruction:** `MusE` owns these toolbars exclusively — they are
its `QObject` children (added via `QMainWindow::addToolBar()` inside
`MusE::MusE()`, never reparented). `MusE`'s own destructor deletes them.
`TopWin` must **never** delete them, even though they sit in its own
`_toolbars` list.

To make that safe, `TopWin` tracks which of its `_toolbars` entries are
borrowed:

```cpp
std::list<QToolBar*> _borrowedToolbars;   // cobject.h
bool isBorrowedToolBar(QToolBar* tb) const;
```

- `TopWin::~TopWin()` skips `delete` for anything in `_borrowedToolbars`.
- `TopWin::shareToolsAndMenu(bool)` skips reparenting (`removeToolBar`/`setParent(nullptr)`
  in one direction, `addToolBar`/`show()` in the other) for borrowed toolbars —
  they must stay parented to `MusE` at all times, never move to the editor
  window.

**`MusE::setCurrentMenuSharingTopwin()` no-op case:** this function's whole
job used to be swapping `MusE`'s toolbar for the active `TopWin`'s own copy
(`insertToolBar` + `removeToolBar`). For shared toolbars, `tb` (MusE's) and
`atb` (from `win->toolbars()`) are now *literally the same object*. The
function detects `tb == atb` and treats it as a no-op — already in place,
already shown, nothing to swap — instead of removing the toolbar out from
under itself.

## 2. Non-shared, per-window toolbars

**Toolbars:** everything specific to one editor type and never meant to be
seen elsewhere — e.g. `Arranger`'s own toolbar, `EditToolBar` ("Edit Tools"),
`AutomationModeToolBar`, `VisibleTracks`, `PartColorToolbar` (all built in
`ArrangerView::ArrangerView()`, not `TopWin`).

**Owner / constructor:** the specific `TopWin` subclass itself. Fully owned,
normally constructed, normally destroyed (via the existing, unmodified
`TopWin::addToolBar()` path and `~TopWin()`'s delete loop).

There is currently no toolbar left in the "genuinely needs a distinct
instance per window, but is otherwise generic" category — Tempo/Signature/
Position used to be there (due to `this`-bound signal wiring) but were moved
to the shared category once their routing became dynamic (see above).

## Files and classes involved

| File | Class | Role |
|---|---|---|
| `app.h` / `app.cpp` | `MusE` | Builds and owns every shared toolbar exactly once. Exposes `sharedOptionalToolBar(objName)` and `getActiveTopWin()`. Implements the `setCurrentMenuSharingTopwin()` swap (and its `tb==atb` no-op case). |
| `cobject.h` / `cobject.cpp` | `TopWin` | Base class of every editor window. Constructor borrows shared toolbars from `MusE` instead of building them; still builds its own genuinely unique toolbars normally. Tracks `_borrowedToolbars` so its destructor and `shareToolsAndMenu()` never touch `MusE`'s toolbars. |
| `arrangerview.h` / `arrangerview.cpp` | `ArrangerView : TopWin` | Adds its own unique toolbars (Edit Tools, Automation Mode, Visible Track Types, Part Colors) on top of what it inherits from `TopWin`. |
| `midieditor.h` / `midieditor.cpp` | `MidiEditor : TopWin` | Base for Pianoroll/DrumEdit/WaveEdit/ScoreEdit-style editors. Gets the shared toolbars "for free" via `TopWin`; before this fix, each such editor built its own separate Tempo/Signature/Position copy. |
| `sig_tempo_toolbar.h/.cpp` | `TempoToolbar`, `SigToolbar` | The toolbar widget classes themselves. Unaware of sharing — they don't know or care whether they're shared; that's entirely managed by `MusE`/`TopWin`. |

## Who is responsible for what (summary)

- **Construction:** `MusE::MusE()` only, for every shared toolbar. Each
  `TopWin` subclass only constructs what's genuinely unique to it.
- **Showing/hiding:** `MusE`'s toolbar area shows the currently-relevant one;
  `TopWin::shareToolsAndMenu()` never touches shared toolbars at all now.
- **Destruction:** `MusE`'s destructor only. `TopWin::~TopWin()` explicitly
  skips anything marked borrowed.
- **Signal routing for shared toolbars:** wired once in `MusE::MusE()`,
  dynamically dispatched via `MusE::activeTopWin` + virtual `focusCanvas()`.

## Other noteworthy points

- **Construction-order dependency:** the whole scheme relies on `MusE`'s own
  toolbars existing before the first `TopWin` is built. This holds today
  (verified: `arrangerView` is constructed near the end of `MusE::MusE()`,
  after all toolbar construction), but would break if that order were ever
  changed — there is no compile-time guard against it, only the `debugMsg`
  warning printed by `TopWin::TopWin()` if `sharedOptionalToolBar()` returns
  `nullptr`.
- **Debug logging:** `TopWin::addToolBar()` and `TopWin::shareToolsAndMenu()`
  log (behind `MusEGlobal::heavyDebugMsg`, enabled via `-D -D` on the command
  line — see `main.cpp`) which branch runs per toolbar; `MusE::setCurrentMenuSharingTopwin()`
  logs insert/remove/no-op decisions the same way. Useful for verifying the
  swap/no-op behavior at runtime.
- **Name matching is still string-based.** Both sides identify a toolbar
  purely by `objectName()` (e.g. `"Transport tool"`). A typo or rename on
  only one side silently breaks either the sharing (falls back to appending a
  second toolbar) or the lookup (falls back to `nullptr`, logged if
  `debugMsg` is on). There is no shared constant/enum for these names.
