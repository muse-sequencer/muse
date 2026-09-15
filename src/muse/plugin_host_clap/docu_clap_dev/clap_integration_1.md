
# CLAP Plugin Integration — MusE Developer Notes

**CLAP** (CLever Audio Plugin) is a modern open plugin standard by free-audio.
This document describes how CLAP support was added to MusE, which files were
modified, and how each piece fits into the existing plugin infrastructure.

Reference implementations used:
- `src/muse/dssihost.cpp` — host pattern to mirror
- `https://github.com/free-audio/clap-host` — CLAP host reference

---


## Architecture Overview

CLAP integration follows the same layered structure as DSSI:

```
muse_plugin_scan (subprocess, src/sandbox/)
    └── loadPluginLib()
            └── writeClapInfo()          ← new, serialises descriptor to XML cache
                    ↓
        plugin cache XML on disk
                    ↓
MusE main process reads cache → PluginScanList
                    ↓
        initCLAP()  [main.cpp]
                    ↓
        ClapSynth   [clap_host.cpp]      ← one per plugin id in scan list
                    ↓
        ClapSynthIF [clap_host.cpp]      ← one per active track instance
```

The sandbox subprocess (`muse_plugin_scan`) loads each `.clap` file in isolation
so a crashing plugin cannot bring down MusE. Results are written as XML to a
per-file cache and read back by the main process.

---


## Inheritance / Class Relationships

```
PluginBase          (src/muse/plugin.h)
  ├── Plugin        — LADSPA/DSSI rack plugins (has _portCount, _inports etc.)
  └── Synth         (src/muse/synth.h)
        ├── MessSynth
        ├── DssiSynth
        └── ClapSynth    ← new, owns _portCount/_inports etc. directly
                           (these fields are NOT in PluginBase or Synth)

SynthIF             (src/muse/synth.h) : PluginIBase
  ├── MessSynthIF
  ├── DssiSynthIF
  └── ClapSynthIF  ← new
```

**Important:** `_portCount`, `_inports`, `_outports`, `_controlInPorts`,
`_controlOutPorts` live in `Plugin`, not in `PluginBase`/`Synth`. `DssiSynth`
declares them directly, and so does `ClapSynth`.

`_hasGui` does not exist anywhere in the base classes. `ClapSynthIF::hasGui()`
returns `_extGui != nullptr` (i.e. whether the plugin advertises `CLAP_EXT_GUI`).

`SynthIF` does **not** have `getParameterOut()` — only `paramOut()` via
`PluginIBase`. Do not add `getParameterOut` to `ClapSynthIF`.

---


## New Files

### `src/muse/clap_host.h` + `src/muse/clap_host.cpp`

The main host implementation. Contains:

- **`ClapSynth`** — represents one plugin descriptor (factory metadata).
  Manages library load/unload with reference counting, mirrors `DssiSynth`.
  Declares its own `_portCount`, `_inports`, `_outports`, `_controlInPorts`,
  `_controlOutPorts` (not inherited).

- **`ClapSynthIF`** — one running instance per active synth track.
  Owns the `clap_plugin_t*`, audio buffers, parameter arrays, event buffers,
  and the per-instance `clap_host_t` vtable with `host_data = this`.
  Implements all `SynthIF` pure virtuals including `hasGui()`, `hasNativeGui()`,
  `eventsPending()`.

- **`initCLAP()`** — walks `MusEPlugin::pluginList`, creates a `ClapSynth`
  for every `PluginTypeCLAP` entry, pushes it into `MusEGlobal::synthis`.

### `src/libs/plugin/plugin_cache_writer_clap.cpp`

Implements `MusEPlugin::writeClapInfo()`. Called from `muse_plugin_scan` to
serialise a CLAP factory's descriptors into the MusE XML plugin cache format.
The function is declared in `plugin_cache_writer.h` inside `namespace MusEPlugin`,
alongside the existing `writeDssiInfo`, `writeMessInfo` etc.

---



## Modified Files

### 1. `src/libs/plugin/plugin_scan.h`

Add `PluginTypeCLAP` to the `PluginType` bitmask enum and update `PluginTypesAll`.
These are power-of-2 bitmask flags — not sequential integers.

**Before:**
```cpp
enum PluginType {
  // ...
  PluginTypeMETRONOME  = 0x080,
  PluginTypeUnknown    = 0x100,
};
static const PluginTypes_t PluginTypesAll = 0x1ff;
```

**After:**
```cpp
enum PluginType {
  // ...
  PluginTypeMETRONOME  = 0x080,
  PluginTypeCLAP       = 0x100,   // ← new
  PluginTypeUnknown    = 0x200,
};
static const PluginTypes_t PluginTypesAll = 0x3ff;  // ← updated
```

---

### 2. `src/libs/plugin/plugin_scan.cpp`

Add CLAP to the human-readable type string map used by the XML cache and UI.

**After the existing METRONOME entry:**
```cpp
{PluginTypeMETRONOME, QT_TRANSLATE_NOOP("MusEPlugin", "Metronome")},
{PluginTypeCLAP,      QT_TRANSLATE_NOOP("MusEPlugin", "CLAP")},    // ← new
{PluginTypeUnknown,   QT_TRANSLATE_NOOP("MusEPlugin", "Unknown")}
```

---

### 3. `src/libs/plugin/plugin_cache_writer.h`

Declare `writeClapInfo()` inside `namespace MusEPlugin`, after the existing
`writeDssiInfo` declaration. The include for `<clap/clap.h>` goes at the top
of the file with the other conditional includes.

**At top of file, with other conditional includes:**
```cpp
#ifdef CLAP_SUPPORT
#include <clap/clap.h>
#endif
```

**Inside `namespace MusEPlugin`, after `writeDssiInfo`:**
```cpp
#ifdef CLAP_SUPPORT
bool writeClapInfo(const char* filename, const clap_plugin_entry_t* entry,
                   bool do_ports, int level, MusECore::Xml& xml);
#endif
```

**Pitfalls:**
- The declaration must be **inside** `namespace MusEPlugin` — placing it before
  the `namespace MusECore { class Xml; }` forward declaration causes a compile
  error (`MusECore::Xml` not declared).
- The return type must match the definition (`bool`, not `void`).
- Do not add a second declaration inside `plugin_cache_writer_clap.cpp` — the
  header declaration is sufficient.

---



### 4. `src/libs/plugin/plugin_cache_writer_clap.cpp` (new file)

```cpp
#include "config.h"
#ifdef CLAP_SUPPORT
#include <clap/clap.h>
#include <clap/factory/plugin-factory.h>
#include "plugin_cache_writer.h"
#include "plugin_scan.h"
#include "xml.h"

namespace MusEPlugin {

bool writeClapInfo(const char* filename, const clap_plugin_entry_t* entry,
                   bool /*do_ports*/, int level, MusECore::Xml& xml)
{
  entry->init(filename);
  const clap_plugin_factory_t* factory = static_cast<const clap_plugin_factory_t*>(
    entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
  if(!factory) { entry->deinit(); return false; }

  const uint32_t count = factory->get_plugin_count(factory);
  for(uint32_t i = 0; i < count; ++i)
  {
    const clap_plugin_descriptor_t* desc = factory->get_plugin_descriptor(factory, i);
    if(!desc || !desc->id || !desc->name) continue;

    PluginClass_t cls = PluginClassNone;
    if(desc->features)
      for(int f = 0; desc->features[f]; ++f)
      {
        if(strcmp(desc->features[f], CLAP_PLUGIN_FEATURE_INSTRUMENT) == 0)
          cls = PluginClass_t(cls | PluginClassInstrument);
        if(strcmp(desc->features[f], CLAP_PLUGIN_FEATURE_AUDIO_EFFECT) == 0)
          cls = PluginClass_t(cls | PluginClassEffect);
      }
    if(cls == PluginClassNone) cls = PluginClassInstrument;

    xml.tag(level++, "plugin");
    xml.strTag(level, "type",        pluginTypeToString(PluginTypeCLAP));
    xml.strTag(level, "class",       pluginClassToString(cls));
    xml.strTag(level, "uri",         desc->id);
    xml.strTag(level, "label",       desc->id);
    xml.strTag(level, "name",        desc->name        ? desc->name        : "");
    xml.strTag(level, "description", desc->description ? desc->description : "");
    xml.strTag(level, "maker",       desc->vendor      ? desc->vendor      : "");
    xml.strTag(level, "version",     desc->version     ? desc->version     : "");
    xml.intTag(level, "portCount",       0);
    xml.intTag(level, "inports",         0);
    xml.intTag(level, "outports",        0);
    xml.intTag(level, "controlInPorts",  0);
    xml.intTag(level, "controlOutPorts", 0);
    xml.intTag(level, "pluginFlags",     int(PluginHasGui));
    xml.tag(--level, "/plugin");
  }
  entry->deinit();
  return true;
}

} // namespace MusEPlugin
#endif // CLAP_SUPPORT
```

---

### 5. `src/libs/plugin/CMakeLists.txt`

Add `plugin_cache_writer_clap.cpp` to the writer module source list, and
enable the `CLAP_SUPPORT` compile definition on that target so `config.h`
and the `#ifdef` guards work correctly.

```cmake
file(GLOB plugin_cache_writer_source_files
      plugin_cache_writer.cpp
      )

if(CLAP_SUPPORT)
      list(APPEND plugin_cache_writer_source_files
            plugin_cache_writer_clap.cpp
            )
endif(CLAP_SUPPORT)

# ... add_library(plugin_cache_writer_module ...) unchanged ...

if(CLAP_SUPPORT)
      target_compile_definitions(plugin_cache_writer_module PRIVATE CLAP_SUPPORT)
endif(CLAP_SUPPORT)
```

**Note:** `CLAP_SUPPORT` must be set as `CACHE INTERNAL` in the top-level
`CMakeLists.txt` (see §11) for it to be visible in subdirectory `CMakeLists.txt`
files. A plain `set(CLAP_SUPPORT ON)` without `CACHE INTERNAL` is invisible
to child directories.

---

### 6. `src/sandbox/muse_plugin_scan.cpp`

This subprocess opens one plugin file at a time and tries each known entry
point in order (DSSI → MESS → LADSPA → LinuxVST → CLAP). Add CLAP detection
after the LinuxVST block, before the `!found` fallback.

**Add includes at top:**
```cpp
#ifdef CLAP_SUPPORT
#include <clap/clap.h>
#include <clap/factory/plugin-factory.h>
#endif
```

**Add detection block inside `loadPluginLib()`, after the LinuxVST block:**
```cpp
#ifdef CLAP_SUPPORT
    if(!found)
    {
      if(types & MusEPlugin::PluginTypeCLAP)
      {
        const clap_plugin_entry_t* entry =
          reinterpret_cast<const clap_plugin_entry_t*>(
            qlib.resolve("clap_entry"));
        if(entry)
        {
          DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Is a CLAP library\n");
          MusECore::Xml xml(&outfile);
          xml.header();
          int level = 0;
          level = xml.putFileVersion(level);
          MusEPlugin::writeClapInfo(filename, entry, do_ports, level, xml);
          xml.tag(1, "/muse");
          found = true;
        }
        else
        {
          DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Not a CLAP library...\n");
        }
      }
    }
#endif // CLAP_SUPPORT
```

Unlike DSSI/LADSPA, the library stays loaded while `writeClapInfo()` runs
because `clap_plugin_entry_t*` is only valid while the library is open.
`writeClapInfo()` calls `entry->init()` / `entry->deinit()` internally.

---

### 7. `src/sandbox/CMakeLists.txt`

Pass the `CLAP_SUPPORT` definition to the scanner subprocess. The include path
is not needed since clap headers are in the system path.

```cmake
if(CLAP_SUPPORT)
  target_compile_definitions(muse_plugin_scan PRIVATE CLAP_SUPPORT)
endif()
```

**Note:** Do not use `ENABLE_CLAP` here — use `CLAP_SUPPORT`, which is the
cached variable set after the header check. `ENABLE_CLAP` is the user-facing
option; `CLAP_SUPPORT` is what indicates the headers were actually found.

---

### 8. `src/muse/globals.h`

Add the `loadCLAP` flag alongside `loadDSSI`, `loadLV2` etc.

```cpp
// near line 117, alongside the other load* flags:
extern bool loadCLAP;
```

---

### 9. `src/muse/globals.cpp`

Default to enabled, same as all other plugin types.

```cpp
// near line 112:
bool loadCLAP = true;
```

---

### 10. `src/muse/main.cpp`

Two locations (one init block was already handled by the author).

**~Line 528** — bulk-disable block (safe/no-plugin mode):
```cpp
MusEGlobal::loadDSSI  = false;
MusEGlobal::loadCLAP  = false;   // ← add
```

**~Line 1294** — primary init block:
```cpp
        if(MusEGlobal::loadDSSI)
              MusECore::initDSSI();
  #ifdef CLAP_SUPPORT
        qDebug() << "->" << qPrintable(QTime::currentTime().toString("hh:mm:ss.zzz"))
                 << "Init CLAP plugins...";
        if(MusEGlobal::loadCLAP)
              MusECore::initCLAP();
  #endif
  #ifdef LV2_SUPPORT
        // ... existing LV2 block unchanged ...
  #endif
```

---

### 11. `src/muse/plugin.cpp`

~Line 1862 — plugin browser / "Add Synth" type filter. Mirror the `loadDSSI`
check that gates which types appear in the browser:

```cpp
        if(MusEGlobal::loadDSSI)
          types |= MusEPlugin::PluginTypeDSSI | MusEPlugin::PluginTypeDSSIVST;
#ifdef CLAP_SUPPORT
        if(MusEGlobal::loadCLAP)
          types |= MusEPlugin::PluginTypeCLAP;
#endif
```

---

### 12. `src/muse/components/aboutbox_impl.cpp`

Debug info dialog — add CLAP to the displayed flags list.

```cpp
internalDebugInformation->append(
    QString("loadDSSI:\t\t%1").arg(MusEGlobal::loadDSSI ? "true" : "false"));
// add:
internalDebugInformation->append(
    QString("loadCLAP:\t\t%1").arg(MusEGlobal::loadCLAP ? "true" : "false"));
```

---

### 13. `src/CMakeLists.txt` (top-level)

CLAP is header-only — no library to link.

```cmake
##
## check for CLAP
##
option(ENABLE_CLAP "Enable CLAP (CLever Audio Plugin) support" ON)

if(ENABLE_CLAP)
      CHECK_INCLUDE_FILE_CXX(clap/clap.h HAVE_CLAP_H)
      if(HAVE_CLAP_H)
            set(CLAP_SUPPORT ON CACHE INTERNAL "")   # ← CACHE INTERNAL required
            message("CLAP support enabled")
      else(HAVE_CLAP_H)
            message("CLAP support disabled (clap/clap.h not found)")
      endif(HAVE_CLAP_H)
else(ENABLE_CLAP)
      message("CLAP disabled")
endif(ENABLE_CLAP)
```

Add to summary and warning sections:
```cmake
summary_add("CLAP support" CLAP_SUPPORT)

if(ENABLE_CLAP AND (NOT CLAP_SUPPORT))
      message("** WARNING: CLAP was enabled, but clap/clap.h was not found.")
      message("** HINT: Install clap development headers (package: clap-dev or clap).")
endif(ENABLE_CLAP AND (NOT CLAP_SUPPORT))
```

### 14. `src/muse/CMakeLists.txt`

Add `clap_host.cpp` to the `core` library source glob:

```cmake
file(GLOB core_source_files
      # ... existing files ...
      dssihost.cpp
      clap_host.cpp    # ← add
      event.cpp
      # ...
      )
```

### 15. `config.h.in`

Add the cmake define (alongside `DSSI_SUPPORT`, `LV2_SUPPORT` etc.):

```c
#cmakedefine CLAP_SUPPORT
```

---

### 16. `src/muse/clap_host.cpp`

- `buildHostVtable()`: replace `PACKAGE_VERSION` → `VERSION` (config.h defines
  `VERSION`, not `PACKAGE_VERSION`)
- `getData()`: no code change needed here — access works once friend declarations
  are added (see §17–18)

---

### 17. `src/muse/synth.h`

Add friend declaration inside `class SynthI`, alongside `DssiSynthIF`:

```cpp
friend class ClapSynthIF;
```

---

### 18. `src/muse/mididev.h`

Add friend declaration inside `class MidiDevice`, alongside other friend classes:

```cpp
friend class ClapSynthIF;
```

---

### 19. `src/muse/clap_host.h`

Remove `Q_OBJECT` macro from `ClapSynthIF` — `SynthIF` does not inherit
`QObject` so moc must not process this class.

---

### 20. `src/muse/plugin.cpp`

Five changes, all `#ifdef CLAP_SUPPORT` guarded:

- `writeProperties()` switch (~line 1026): add `case PluginTypeCLAP:` to the
  list of types that write a `<type>` XML tag.
- `Plugin::release()` switch (~line 1617): add `case PluginTypeCLAP:` to the
  "not handled here" error group.
- `initPlugins()` switch (~line 1829): add standalone `case PluginTypeCLAP: break;`
  — CLAP goes into `synthis` via `initCLAP()`, not `plugins`.
- `initPlugins()` (~line 1892): **remove** the misplaced `types |= PluginTypeCLAP`
  block that was accidentally placed inside the DSSI case body.
- `PluginI::configure()` switch (~line 4156): add `case PluginTypeCLAP:` to the
  default `break` group.

---

### 21. `src/muse/synth.cpp`, `src/muse/vst_native.cpp`, `src/muse/dssihost.cpp`

Each has a switch in its `init*()` function that enumerates plugin types.
Add in each:

```cpp
#ifdef CLAP_SUPPORT
      case MusEPlugin::PluginTypeCLAP:
        // Registered via initCLAP(), not here.
      break;
#endif
```

---

### 22. `src/muse/components/plugindialog.cpp`

`fillPlugs()` switch (~line 479): same `case PluginTypeCLAP: break;` addition.

---

### 23. `src/muse/components/synthdialog.h`

Add `SEL_TYPE_CLAP` to the `SynthType` enum (guarded by `#ifdef CLAP_SUPPORT`):

```cpp
enum SynthType { SEL_TYPE_MESS, SEL_TYPE_DSSI, SEL_TYPE_LV2, SEL_TYPE_VST,
#ifdef CLAP_SUPPORT
                 SEL_TYPE_CLAP,
#endif
                 SEL_TYPE_ALL };
```

---

### 24. `src/muse/components/synthdialog.cpp`

Two changes:

- Constructor (~line 68): add `ui.pluginType->addItem("CLAP", SEL_TYPE_CLAP);`
  guarded by `#ifdef CLAP_SUPPORT`.
- `fillSynths()` switch (~line 424): add `case PluginTypeCLAP:` with
  `SEL_TYPE_CLAP` filter, guarded by `#ifdef CLAP_SUPPORT`. Place it before
  the `PluginTypeLADSPA` continue-group (not after a duplicate of it).

---

### 25. `src/muse/gconfig.h`

Add to `GlobalConfigValues` struct after `pluginDssiPathList`:

```cpp
#ifdef CLAP_SUPPORT
      QStringList pluginClapPathList;
#endif
```

**Critical:** the position in the struct must exactly match the position in
the initializer lists in `gconfig.cpp` and `musewidgetsplug.cpp`. A mismatch
causes silent struct layout corruption and runtime crashes.

---

### 26. `src/muse/gconfig.cpp`

- Add `#include "config.h"` at the top (required for `#ifdef CLAP_SUPPORT`
  to be visible in the initializer list).
- Add `QStringList(),  // pluginClapPathList` at the matching position:

```cpp
      QStringList(),              // pluginDssiPathList
#ifdef CLAP_SUPPORT
      QStringList(),              // pluginClapPathList
#endif
      QStringList(),              // pluginVstPathList
```

---

### 27. `src/muse/widgets/musewidgetsplug.cpp`

Same initializer addition as `gconfig.cpp`, at the same relative position.
Also add `#include "config.h"` if not already present.

---

### 28. `src/muse/conf.cpp`

Two locations:

**Read** — add after the complete `pluginDssiPathList` else-if block:
```cpp
#ifdef CLAP_SUPPORT
                        else if (tag == "pluginClapPathList")
#if QT_VERSION >= 0x050e00
                              MusEGlobal::config.pluginClapPathList = xml.parse1().split(":", Qt::SkipEmptyParts);
#else
                              MusEGlobal::config.pluginClapPathList = xml.parse1().split(":", QString::SkipEmptyParts);
#endif
#endif // CLAP_SUPPORT
```

**Write** — add after `pluginDssiPathList` write:
```cpp
#ifdef CLAP_SUPPORT
      xml.strTag(level, "pluginClapPathList", MusEGlobal::config.pluginClapPathList.join(":"));
#endif
```

**Pitfall:** do not insert the read block between the `else if (tag == "pluginDssiPathList")`
test and its `#if QT_VERSION` body — that severs the else-if from its
assignment and produces "expected primary-expression before else".

---

### 29. `src/muse/main.cpp`

Add CLAP path initialisation block after the DSSI path block (~line 888):

```cpp
#ifdef CLAP_SUPPORT
        if(MusEGlobal::config.pluginClapPathList.isEmpty())
        {
            MusEGlobal::config.pluginClapPathList
              << QDir::homePath() + "/.clap"
              << "/usr/lib/clap"
              << "/usr/local/lib/clap";
        }
        const QString clap_path = qEnvironmentVariable("CLAP_PATH");
        if(!clap_path.isEmpty())
            MusEGlobal::config.pluginClapPathList = clap_path.split(list_separator, Qt::SkipEmptyParts);
        if(!found && qputenv("CLAP_PATH", MusEGlobal::config.pluginClapPathList.join(list_separator).toLocal8Bit()) == 0)
          fprintf(stderr, "Error setting CLAP_PATH\n");
#endif
```

---

### 30. `src/muse/components/genset.h`

- Add `#include "config.h"` before `ui_gensetbase.h` (required for `ClapTab`
  to be visible).
- Add `ClapTab` to `PathTab` enum:

```cpp
enum PathTab { LadspaTab = 0, DssiTab, VstTab, LinuxVstTab, Lv2Tab,
#ifdef CLAP_SUPPORT
               ClapTab,
#endif
             };
```

---

### 31. `src/muse/components/genset.cpp`

Add `ClapTab` case (guarded by `#ifdef CLAP_SUPPORT`) to all six path-list
switch blocks: `updateSettings()`, `apply()`, `addPluginPath()` (×2 switches),
`editPluginPath()` (×2 switches), `removePluginPath()`, `movePluginPathUp()`,
`movePluginPathDown()`.

---

### 32. `src/muse/components/gensetbase.ui`

Add new tab page inside `pluginPathsTabs` after the LV2 tab:

```xml
<widget class="QWidget" name="pluginClapPathTab">
 <attribute name="title"><string>CLAP</string></attribute>
 <layout class="QVBoxLayout" name="verticalLayout_clap">
  <item>
   <widget class="QLabel" name="label_clap">
    <property name="text">
     <string>Directories containing CLAP plugins (*.clap files).
Standard paths: ~/.clap, /usr/lib/clap, /usr/local/lib/clap</string>
    </property>
    <property name="wordWrap"><bool>true</bool></property>
   </widget>
  </item>
  <item>
   <widget class="QListWidget" name="pluginClapPathList">
    <property name="alternatingRowColors"><bool>true</bool></property>
    <property name="selectionMode">
     <enum>QAbstractItemView::ExtendedSelection</enum>
    </property>
   </widget>
  </item>
 </layout>
</widget>
```

## CMake Variable Pitfalls

| Variable | Type | Purpose |
|---|---|---|
| `ENABLE_CLAP` | cmake option | User-facing ON/OFF toggle |
| `HAVE_CLAP_H` | cached by `CHECK_INCLUDE_FILE_CXX` | Whether header was found |
| `CLAP_SUPPORT` | `CACHE INTERNAL` | Propagated to all subdirectories |

`CLAP_SUPPORT` **must** use `CACHE INTERNAL` — a plain `set(CLAP_SUPPORT ON)`
is only visible in the current `CMakeLists.txt` scope, not in subdirectories
like `src/libs/plugin/` or `src/sandbox/`. Without this, `if(CLAP_SUPPORT)`
in child cmake files silently evaluates to false, the source file is not appended
to the build list, and the linker fails with "undefined reference to writeClapInfo".

Do **not** use `target_compile_definitions(... PRIVATE CLAP_SUPPORT)` in
addition to `config.h` — `config.h` already `#define CLAP_SUPPORT` via
`#cmakedefine`, causing a `-DCLAP_SUPPORT` redefinition warning.

---

## CLAP Plugin Scan Paths

CLAP's standard XDG paths (from the CLAP spec):

| Path | Notes |
|---|---|
| `~/.clap` | user plugins |
| `/usr/lib/clap` | system plugins |
| `/usr/local/lib/clap` | locally installed |

The env var `CLAP_PATH` (colon-separated) overrides/extends these, same
convention as `DSSI_PATH` and `LV2_PATH`.

Add a `clapPluginPath` config variable in `gconfig.h`/`gconfig.cpp` and feed
it to the scanner alongside the existing `dssiPluginPath`.

---

## Key Design Decisions

**Per-instance `clap_host_t` vtable** — Unlike DSSI where the host is
stateless, CLAP callbacks carry a `host_data` pointer. We set `host_data = this`
(`ClapSynthIF*`) in `buildHostVtable()`, so every static C trampoline can
retrieve the instance without a global map.

**Event buffer pool** — CLAP `process()` takes `clap_input_events_t` /
`clap_output_events_t` vtables rather than a flat array. We allocate a
fixed-size pool of `clap_event_midi_t`-sized slots (256 per block) and build
lightweight vtable structs over them as C++ lambdas stored in the instance.
This avoids heap allocation in the audio thread.

**Port counts deferred** — Unlike LADSPA/DSSI, CLAP audio port counts require
the plugin to be instantiated (via `clap_plugin_audio_ports` extension). The
XML cache stores 0 for port counts; they are filled in `ClapSynthIF::init()`
when the plugin is first instantiated for a track.

**`hasGui()` implementation** — There is no `_hasGui` field in `PluginBase`
or `Synth`. `ClapSynthIF::hasGui()` returns `_extGui != nullptr`, i.e. whether
the plugin reported support for `CLAP_EXT_GUI` after `plugin->init()`.

**`Q_OBJECT` not used** — `SynthIF` does not inherit `QObject`, so `Q_OBJECT`
must not appear in `ClapSynthIF`. GUI callbacks use the `clap_host_gui_t`
trampoline instead of Qt signals.

**GUI** — The plugin owns its window contents. The host provides a native
window handle via `gui->set_parent(&clap_window)`. Floating GUIs use
`gui->set_transient()`. A `setParentWindow(WId)` method is needed for
embedded GUI support (not yet implemented).

**State save/load** — `CLAP_EXT_STATE` is detected in `init()` but
`ClapSynthIF::write()` is currently a stub. A full implementation needs a
`clap_ostream_t` backed by `QByteArray`, base64-encoded into the MusE XML
song file, mirroring the DSSI VST chunk approach.

---

## Bugs Fixed During Integration

### 1. `PACKAGE_VERSION` undeclared in `buildHostVtable()` (`clap_host.cpp` line 494)

`config.h` defines `VERSION` (e.g. `"4.2.1"`), not `PACKAGE_VERSION`.

```cpp
// Wrong:
_clapHost.version = PACKAGE_VERSION;
// Correct:
_clapHost.version = VERSION;
```

### 2. Protected member access in `getData()` (`clap_host.cpp` lines 1223–1273)

`ClapSynthIF` accesses `stopFlag()`, `setStopFlag()`, `eventBuffers()`,
`_outUserEvents`, `_outPlaybackEvents` on a `SynthI*` — all declared `protected`
in `MidiDevice` / `SynthI`. Fix: add friend declarations mirroring `DssiSynthIF`.

**`src/muse/synth.h`** — inside `class SynthI`:
```cpp
friend class ClapSynthIF;
```

**`src/muse/mididev.h`** — inside `class MidiDevice`:
```cpp
friend class ClapSynthIF;
```

### 3. `Q_OBJECT` in `ClapSynthIF` (`clap_host.h`)

`SynthIF` does not inherit `QObject`, so `Q_OBJECT` must not appear in
`ClapSynthIF` (causes silent moc failure / linker issues). Remove it.

### 4. `PluginTypeCLAP` not handled in switches (`plugin.cpp`)

Four switches needed a `case MusEPlugin::PluginTypeCLAP:` arm:

- **Line 1026** `writeProperties()` — add to the list of types that write a
  `<type>` tag (same as all other known types).
- **Line 1617** `Plugin::release()` — add to the "not handled here, error"
  group; `ClapSynth` overrides `release()` so this base version should never
  be reached for CLAP.
- **Line 1829** `initPlugins()` — add a standalone `case PluginTypeCLAP: break;`
  after the DSSI block. CLAP plugins are registered via `initCLAP()` into
  `synthis`, not `plugins`, so no action is needed here.
- **Line 4156** `PluginI::configure()` — add to the default `break` group
  (no special `canLoadControls` override until `CLAP_EXT_STATE` is implemented).

### 5. Misplaced CLAP block inside DSSI case in `initPlugins()` (`plugin.cpp` lines 1892–1895)

The integration doc's §11 snippet for `plugin.cpp` was accidentally pasted
inside the `PluginTypeDSSI/DSSIVST` case body, where `types` is not in scope:

```cpp
// WRONG — inside the DSSI case, 'types' is not declared here:
#ifdef CLAP_SUPPORT
    if(MusEGlobal::loadCLAP)
        types |= MusEPlugin::PluginTypeCLAP;
#endif
```

Remove those lines entirely. The CLAP type filter for the plugin browser belongs
in the browser/add-synth code (§11 of this doc), not in `initPlugins()`.

### 6. `undefined symbol: _ZN10MusEGlobal8loadCLAPE` at runtime

`bool loadCLAP = true;` must be defined in `globals.cpp` (not just declared
`extern` in `globals.h`). If the symbol is missing despite the line being
present, the object file is stale — force recompilation:

```bash
touch src/muse/globals.cpp && ninja -C build
```

### 7. `PluginTypeCLAP` not handled in switches — additional files

Same pattern as bug #4, needed in four more files:

- **`synth.cpp`** `initMidiSynth()` switch — add `case PluginTypeCLAP: break;`
- **`vst_native.cpp`** `initVST_Native()` switch — same
- **`dssihost.cpp`** `initDSSI()` switch — same
- **`plugindialog.cpp`** `fillPlugs()` switch — same

In all cases CLAP is registered via `initCLAP()`, not these functions. Just `break`.

### 8. Duplicate `case PluginTypeLADSPA` in `synthdialog.cpp`

The `#ifdef CLAP_SUPPORT` CLAP case was inserted before the existing
`PluginTypeLADSPA` continue-group, leaving a stale second copy of that case.
Remove the duplicate `case MusEPlugin::PluginTypeLADSPA:` line. Final order:

```cpp
#ifdef CLAP_SUPPORT
          case MusEPlugin::PluginTypeCLAP:
            if(selType != SEL_TYPE_CLAP && selType != SEL_TYPE_ALL)
                continue;
          break;
#endif
          case MusEPlugin::PluginTypeLADSPA:
          case MusEPlugin::PluginTypeVST:
          ...
            continue;
```

### 9. `conf.cpp` — CLAP read block inserted inside DSSI else-if body

The `pluginClapPathList` read block was inserted between the `else if (tag ==
"pluginDssiPathList")` test and its `#if QT_VERSION` body, producing
"expected primary-expression before else":

```cpp
// WRONG:
else if (tag == "pluginDssiPathList")
// ← CLAP block inserted here, severing the else-if from its body
#if QT_VERSION ...
    MusEGlobal::config.pluginDssiPathList = ...
```

Fix: move the CLAP block to *after* the complete DSSI else-if (after its
`#endif`), as a new `else if` in the chain. Also add a `QT_VERSION` guard
matching the other path list entries.

### 10. `GlobalConfigValues` struct layout mismatch — crash at startup

**Symptom:** SIGSEGV inside `main()` at `config.pluginLv2PathList.isEmpty()`
or `config.userInstrumentsDir.isEmpty()`, with `audioType` showing a garbage
value like `1433129664`. `nm` on the installed `libmuse_core.so` shows no
`pluginClapPath` symbol even after `ninja install`.

**Cause:** `gconfig.cpp` does not include `config.h`, so the
`#ifdef CLAP_SUPPORT` around `QStringList pluginClapPathList` evaluates false
and the field is omitted from the struct in that TU. The binary `muse4` is
compiled with the field present (it includes `config.h` transitively), but
`libmuse_core.so` is missing it — the two disagree on struct layout and every
field after `pluginDssiPathList` is offset by 24 bytes.

**Fix:** Add `#include "config.h"` at the top of `gconfig.cpp` (and verify
`musewidgetsplug.cpp` also includes it). After adding, do a full clean rebuild:

```bash
rm -rf build && mkdir build && cd build
cmake .. && ninja && sudo ninja install
```

**Verify** the symbol is present after install:
```bash
nm -D /usr/local/lib/muse-4.2/modules/libmuse_core.so | grep pluginClapPath
```

### 11. `genset.h` — `ClapTab` enum invisible without `config.h`

`genset.h` did not include `config.h`, so the `#ifdef CLAP_SUPPORT` guard
around `ClapTab` in the `PathTab` enum evaluated false, making `ClapTab`
undeclared in `genset.cpp`:

```
error: 'ClapTab' was not declared in this scope
```

Fix: add `#include "config.h"` as the first include in `genset.h`, before
`ui_gensetbase.h`.

---

## GUI — CLAP path settings (implemented)

The plugin search path UI in Settings → Global Settings → Paths tab now
includes a CLAP tab. Files modified:

- **`genset.h`** — added `ClapTab` to `PathTab` enum (guarded by
  `#ifdef CLAP_SUPPORT`); added `#include "config.h"`
- **`genset.cpp`** — added `ClapTab` cases to all six path-list switch blocks:
  `updateSettings()`, `apply()`, `addPluginPath()`, `editPluginPath()`,
  `removePluginPath()`, `movePluginPathUp()`, `movePluginPathDown()`
- **`gensetbase.ui`** — added `pluginClapPathTab` / `pluginClapPathList`
  widget after the LV2 tab in `pluginPathsTabs`
- **`gconfig.h`** — added `QStringList pluginClapPathList` field to
  `GlobalConfigValues` (guarded by `#ifdef CLAP_SUPPORT`)
- **`gconfig.cpp`** — added `QStringList()` initializer at matching position;
  added `#include "config.h"`
- **`musewidgetsplug.cpp`** — same initializer addition
- **`conf.cpp`** — read and write of `pluginClapPathList` XML tag
- **`main.cpp`** — default path population and `CLAP_PATH` env var handling

## GUI — Synth dialog type filter (implemented)

- **`synthdialog.h`** — added `SEL_TYPE_CLAP` to `SynthType` enum
- **`synthdialog.cpp`** — added "CLAP" combo item and `PluginTypeCLAP` case
  in `fillSynths()` switch

---

## TODO / Not Yet Implemented

- `ClapSynthIF::write()` — state serialisation via `CLAP_EXT_STATE`
- `setParentWindow(WId)` — embedded GUI support (floating works already)
- Transport info in `clap_process_t.transport`
- `CLAP_EXT_PRESET_LOAD` for `populatePatchPopup()`
- Sysex passthrough (no sysex event in CLAP core spec; vendor extension needed)
- `hostRequestRestart()` — post to MusE main event queue properly
- `hostRequestCallback()` — schedule `on_main_thread()` from main thread

---
---

# Session Update — Audio, GUI, Build Consolidation

This section supersedes earlier notes where they conflict. It records the
relocation of the CLAP host into its own subdirectory, the bugs fixed to get
audio + custom GUI working, and how the build flag was consolidated. Where an
earlier section above describes a now-replaced approach (e.g. the original
`strTag`-based `writeClapInfo`, or per-target `-DCLAP_SUPPORT`), the version
here is authoritative.

## Directory / file layout (current)

CLAP host sources were moved out of `src/muse/` into their own subdirectory so
they don't crowd the main tree:

```
src/muse/plugin_host_clap/
    clap_host.h          host classes (ClapSynth, ClapSynthIF), shared helpers
    clap_host.cpp        non-GUI: init, audio process, events, params, host vtable
    clap_host_gui.cpp    GUI only: window embedding, size negotiation, gui trampolines
    clap_host.md         (notes)
    clap_integration.md  (this file)
```

`writeClapInfo()` remains in `src/libs/plugin/plugin_cache_writer_clap.cpp`.

### Why clap_host_gui.cpp is split out

A C++ class may have its methods defined across multiple translation units.
All `ClapSynthIF` GUI methods (`showNativeGui`, `closeNativeGui`,
`nativeGuiVisible`, `hostGuiClosed`, `hostGuiRequestResize`, `destroyGui`) plus
the `clap_host_gui` trampolines and `s_hostGuiExt` live in `clap_host_gui.cpp`,
keeping Qt-widget dependencies (`QWidget`, `QGuiApplication`) out of the audio
core. `clap_host.cpp` reaches the GUI vtable through a small accessor
`clapGuiHostExt()` (declared in the header). `hostFromClap()` is a
`static inline` in the header so both TUs share one definition.

## CMake (current)

`src/muse/CMakeLists.txt`:
- `core_source_files` lists `plugin_host_clap/clap_host.cpp` and
  `plugin_host_clap/clap_host_gui.cpp` (replacing the old `clap_host.cpp`).
- `include_directories(...)` gains `${CMAKE_CURRENT_SOURCE_DIR}` (so the
  relocated files still find `synth.h`, `audio.h`, … in `src/muse`) and
  `plugin_host_clap`.
- The CLAP block keeps `include_directories(${CLAP_INCLUDE_DIR})` but **no
  longer** sets `target_compile_definitions(core PRIVATE CLAP_SUPPORT)` — see
  the build-flag section below.

## CLAP_SUPPORT — single source of truth

Earlier the macro was delivered twice: via `config.h` (`#cmakedefine
CLAP_SUPPORT`, driven by the cache var in `src/CMakeLists.txt`) **and** via
per-target `target_compile_definitions(... CLAP_SUPPORT)` in
`src/muse`, `src/libs/plugin`, `src/sandbox`. The two collided as a
`-DCLAP_SUPPORT` vs empty-body `#define` mismatch → "CLAP_SUPPORT redefined".

**Resolution:** keep `config.h` as the *only* delivery channel and remove all
three `target_compile_definitions(... CLAP_SUPPORT)` lines. Every CLAP TU
already includes `config.h`, so the macro still reaches all of them. The cache
var stays in `src/CMakeLists.txt`; the `if(CLAP_SUPPORT)` guards in subdir
CMake files still work (they test the variable, not the compile flag).

**Hard requirement that this exposed:** any file that gates its whole body (or
an enum entry) on `#ifdef CLAP_SUPPORT` MUST `#include "config.h"` *before* the
guard, otherwise the macro isn't defined yet and the guarded code silently
vanishes. Files corrected for this:
- `plugin_cache_writer_clap.cpp` — `#include "config.h"` before `#ifdef CLAP_SUPPORT`
  (without it, `writeClapInfo` compiled to nothing → undefined-reference link error).
- `components/plugindialog.h` — `#include "config.h"` first (without it,
  `SEL_TYPE_CLAP` was dropped from the enum while the `.cpp` case stayed active
  → "SEL_TYPE_CLAP not declared").

Audit command for the same latent bug elsewhere:
```sh
grep -rL '#include "config.h"' $(grep -rl 'CLAP_SUPPORT' src --include=*.h)
```

## Bugs fixed this session

### A. Empty `clap_plugins.scan` — `writeClapInfo` format mismatch
The original `writeClapInfo` hand-wrote XML with `strTag("type", "CLAP")` and
`strTag("class", "Synth")`, but the reader parses `<type>`/`<class>` with
`parseInt()` → both came back 0 (`PluginTypeNone`). `writePluginCacheFile`
filters by `info._type & types`, so every CLAP entry was discarded → empty
cache. It also wrote `file`/`label` as child tags, but the reader reads them as
attributes of `<plugin>`.
**Fix:** `writeClapInfo` now fills a `PluginScanInfoStruct`
(`setPluginScanFileInfo` + type/class/uri/label/name/maker/version) and calls
`writePluginScanInfo()` — the same path `writeDssiInfo`/`writeMessInfo` use —
guaranteeing the exact format the reader expects.

### B. "synth instantiation error" — CLAP routed into DssiSynth
`initDSSI()` (`dssihost.cpp`) had `case PluginTypeCLAP:` with a commented-out
`//break;`, so CLAP entries fell through into the DSSI case and were wrapped in
`DssiSynth` (typed CLAP). `DssiSynth::reference()` then failed on the CLAP id,
and `initCLAP()` skipped the real `ClapSynth` as a duplicate.
**Fix:** give CLAP its own `case ...PluginTypeCLAP: break;` *before* the DSSI
cases so it cannot fall through. (`initVST_Native()` and `initMidiSynth()` were
likewise made to no-op for CLAP.)

### C. No audio — event-buffer slot size
The input/output event pool spaced slots at `sizeof(clap_event_midi_t)`
(~24 B), but note events (`clap_event_note_t`, ~40 B) and param events
(`clap_event_param_value_t`, ~48 B) are larger. Multi-event blocks overran each
other and `get()`/`try_push` indexed into garbage → mangled note-ons → silence.
**Fix:** a `union ClapEventSlot { header; note; midi; param; }` and
`kClapEventSlotSize = sizeof(ClapEventSlot)` used as the stride everywhere
(allocation, write offsets, `get`, `try_push`).

### D. `CLAP_PROCESS_ERROR` — single-buffer audio layout
The host collapsed all channels into one `clap_audio_buffer_t` and hardcoded
`audio_inputs_count/outputs_count = 1`. CLAP is port-based: the plugin expects
one buffer per declared port. Plugins with more than one audio port (Surge XT)
rejected the call.
**Fix:** store per-port channel layout at init (`_inPortChans`,
`_outPortChans`), and in `getData()` build one `clap_audio_buffer_t` per port
(slicing the flat channel arrays), with `audio_*_count` = real port counts.
Added `sortInEvents()` (insertion sort by `header.time`) before `process()`
because CLAP requires a time-ordered input event stream.

### E. Custom GUI — embedding + size, and empty window on Wayland
`showNativeGui()` originally called `create()`+`show()` with no `set_parent()`
and no sizing. Now: query the plugin via `is_api_supported()`, `create()` the
correct (embedded/floating) mode, for embedded create a native `QWidget`, call
`set_scale()`, `get_size()`, size the window (fixed vs resizable per
`can_resize()`), `set_parent()` with the X11 window id, then `show()`. The
plugin-driven `hostGuiRequestResize()` now resizes the host window.
On a **native Wayland** session `winId()` is a Wayland surface, not an X11
window, so embedding yields an empty container; the code detects Wayland
(`QGuiApplication::platformName()`) and prefers the floating path, or prints a
hint to run under XWayland (`QT_QPA_PLATFORM=xcb`) when the plugin only offers
embedded X11.

### F. Switch/enum warnings tidied
- `components/plugindialog.h` — added guarded `SEL_TYPE_CLAP`; `.cpp` case
  qualified `MusEPlugin::PluginTypeCLAP`; `plugInstanceType` initialized to
  `SEL_TYPE_ALL` (the switch has no default).
- `vst_native.cpp` `initVST_Native()` — added guarded
  `case MusEPlugin::PluginTypeCLAP:` to the no-op group (`-Wswitch`).
- `libs/time_stretch/time_stretch.cpp` `normalizeListFrames()` — initialized
  the `prev*` accumulators (frames to 0.0, ratios to 1.0) to clear
  `-Wmaybe-uninitialized` false positives.

(Pre-existing, non-CLAP warnings left as-is: `scoreedit.cpp` array-bounds,
`ctrl.cpp`/`audiotrack.cpp`/`rasterizer.cpp` maybe-uninitialized, plus external
glib/libinstpatch deprecation noise.)

---

# How to support effect-type CLAP plugins (effect rack)

**Current state:** CLAP plugins are registered only as *synths* — `initCLAP()`
creates a `ClapSynth` and pushes it into `MusEGlobal::synthis`. They appear in
the "Add Synth Track" dialog and run as instrument tracks. They do **not**
appear in the per-track **effects rack**, even when their CLAP features list
`audio-effect`, because the rack is populated from `MusEGlobal::plugins`
(the `Plugin`/`PluginI` world), which nothing CLAP adds to.

This is exactly how native VST is wired: `initVST_Native()` registers *both* a
`VstNativeSynth` into `synthis` **and** a `VstNativePluginWrapper` (a `Plugin`
subclass) into `MusEGlobal::plugins`. CLAP needs the same second half.

## What to build

### 1. `ClapPlugin` : `MusECore::Plugin`
A wrapper exposing a CLAP plugin to the effect-rack/`PluginI` machinery, modelled
on `VstNativePluginWrapper` (`vst_native.h/.cpp`). It must implement the
`Plugin` virtuals the rack uses:
- `instantiate(PluginI*)` → create a CLAP instance (reuse `ClapSynthIF`'s
  instantiation logic, or factor a shared `ClapInstance` out of `ClapSynthIF`).
- `reference()` / `release()` — library refcount (already on `ClapSynth`).
- `connectPort(handle, port, value)` — map LADSPA-style port indices onto CLAP
  audio ports and parameters. Effects have audio **in** and **out**, so unlike
  the synth path the input ports must be wired to the rack's audio.
- `apply(handle, nframes, latency)` — the effect-context equivalent of
  `ClapSynthIF::getData()`: build per-port `clap_audio_buffer_t`s from the
  connected rack buffers and call `_plugin->process()`. Reuse the
  `ClapEventSlot` pool and `sortInEvents()` logic.
- `range()/portName()/ctrlValueType()/ctrlMode()/defaultValue()/portd()` —
  derive from CLAP `params` extension (mirror the LADSPA fake-descriptor trick
  `VstNativePluginWrapper` uses: build a `_fakeLd` + `_fakePds`).
- `hasNativeGui()/showNativeGui()/nativeGuiVisible()` — delegate to the same
  embedding code in `clap_host_gui.cpp` (factor it so both the synth IF and the
  plugin wrapper can call it).

### 2. Register effects in `initCLAP()`
Today `initCLAP()` does, per scan entry:
```cpp
if(info._type != PluginTypeCLAP) continue;
// ... create ClapSynth, push into synthis
```
Extend it to also add a rack plugin when the descriptor advertises the effect
feature, mirroring VST's `add_plug`/`add_synth` split:
```cpp
const bool is_effect = info._class & MusEPlugin::PluginClassEffect;
const bool is_synth  = info._class & MusEPlugin::PluginClassInstrument;

// Synth track entry (existing behaviour) — keep for instruments, and (as VST
// does) optionally for effects so they can also be used as a synth track.
if((is_synth || is_effect) && !MusEGlobal::synthis.find(...))
    MusEGlobal::synthis.push_back(new ClapSynth(info));

// Effect-rack entry (new) — needs real audio in+out.
if(is_effect && info._inports > 0 && info._outports > 0 &&
   !MusEGlobal::plugins.find(info._type, cbname, uri, label))
    MusEGlobal::plugins.add(new ClapPlugin(info));   // ClapPlugin : Plugin
```

### 3. Port counts at scan vs instantiation
`writeClapInfo` currently stores 0 for port counts (CLAP needs instantiation to
know them). The `is_effect && _inports>0 && _outports>0` test above will
therefore be false at scan time. Options:
- Instantiate briefly during scan to fill real port counts into the cache
  (heavier scan, accurate rack gating), or
- Defer the in/out-port check to instantiation and register the rack plugin
  optimistically for any `audio-effect`, letting `instantiate()` fail
  gracefully if it turns out to have no audio I/O.
The VST path relies on real counts being present in the cache; matching that
means filling port counts in `writeClapInfo` (instantiate-at-scan).

### 4. `plugindialog` already maps CLAP
`fillPlugs()` already maps `PluginTypeCLAP → SEL_TYPE_CLAP`, so once
`ClapPlugin`s are added to `MusEGlobal::plugins` they will show in the effect
dialog with no further dialog changes (the type-filter radio defaults to ALL).

## Effort estimate
The bulk is `ClapPlugin::apply()` (audio process in effect context) and the
parameter/port-descriptor mapping. Both can largely reuse code already written
for `ClapSynthIF` — the cleanest route is to extract a shared `ClapInstance`
(instantiation, audio buffers, event pool, process) used by both
`ClapSynthIF` (synth track) and `ClapPlugin` (effect rack), so the two paths
don't drift.

