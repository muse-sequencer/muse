# MusE Midi Subsystem — Technical Reference

Covers the Alsa-midi / Jack-midi driver internals: setup, naming, storage,
scanning, and auto-connect. Line numbers are approximate (may shift slightly
with future edits) and refer to the versions of these files as of this
conversation.

## 0. Data model overview

**Important, read this first:** all Midi devices — Alsa *and* Jack *and*
synths — live together in **one single global list**:

```cpp
namespace MusEGlobal {
  MusECore::MidiDeviceList midiDevices;   // mididev.cpp:61
}
```

`MidiDeviceList` (`mididev.cpp` ~395-440) is a plain list of `MidiDevice*`
with no type segregation at all — `add()`/`remove()`/`push_back()` don't care
what type goes in. `find(name, typeHint = -1)` defaults to matching **any**
type unless you explicitly pass one. This is not a corner case to remember
occasionally — **every single piece of code that walks `midiDevices` must
explicitly check `deviceType()` if it only wants one kind.** The very first
bug fixed in this conversation (`populateMidiPorts()` assigning Alsa devices
into ports while "preferring" Jack) was exactly this mistake, and the
existing ALSA-only loop in the same function only works *because* it does
check it (`if((*i)->deviceType() != MusECore::MidiDevice::ALSA_MIDI) continue;`).

`MidiDeviceType` (declared in `mididev.h`, not directly available in this
session — reconstructed from usage across `.cpp` files) has at least:
```cpp
enum MidiDeviceType { ALSA_MIDI, JACK_MIDI, SYNTH_MIDI /* , ... */ };
```

Confirmed `MidiDevice` base-class members (from `mididev.cpp`'s `init()` and
usage elsewhere — not a complete list, header wasn't available):
```cpp
QString              _name;                 // -> name(), set via setName()
int                  _rwFlags;              // 1=writable 2=readable 3=both -> rwFlags()
int                  _openFlags;
int                  _port;                 // assigned MidiPort index, or -1 -> midiPort()
QString              _state;                // e.g. "Closed"/"Not ready"/"Unavailable"
bool                 _readEnable, _writeEnable;
LockFreeBuffer<ExtMidiClock>*        _extClockHistoryFifo;
LockFreeMPSCRingBuffer<MidiPlayEvent>* _playbackEventBuffers;
LockFreeMPSCRingBuffer<MidiPlayEvent>* _userEventBuffers;
std::vector<MidiPlayEvent>*          _sysExOutDelayedEvents;
MidiRecFifo*         _recordFifo[MUSE_MIDI_CHANNELS + 1];
std::atomic<bool>    _stopFlag;
// Route lists (see §3/§5): inRoutes(), outRoutes()
```

Three layers, easy to conflate but distinct:

- **`MidiDevice`** (`MidiAlsaDevice`/`MidiJackDevice`) — one real Alsa or Jack
  midi connection point. Lives in the global `MusEGlobal::midiDevices` list.
  Owns the actual `Route` lists (`inRoutes()`/`outRoutes()`) that record what
  it's connected to.
- **`MidiPort`** — a fixed-size array of `MusECore::MIDI_PORTS` slots
  (`MusEGlobal::midiPorts[0..MIDI_PORTS-1]`), each optionally holding one
  `MidiDevice*` (`MidiPort::setMidiDevice()`/`device()`). This indirection is
  what lets a track survive a device being swapped out.
- **`MidiTrack`** — references a `MidiPort` **by index** (`outPort()`), never
  a `MidiDevice` directly.

So the chain for a note leaving a track is: `MidiTrack → port index →
MidiPort → MidiDevice → Route(s) → actual Alsa/Jack port`. Keep this in mind
when reading `populateMidiPorts()` (§6) — it's wiring the middle two layers
together, not tracks directly.

## 1. ALSA setup vs. JACK setup

Both drivers are **not mutually exclusive** — MusE can run either or both at
once. The decision is made once, at startup, in:

**`mididev.cpp:82` — `initMidiDevices()`**
```cpp
#ifdef ALSA_SUPPORT
  if(MusEGlobal::config.enableAlsaMidiDriver ||   // user setting
     MusEGlobal::useAlsaWithJack ||                // -A command line switch
     MusEGlobal::audioDevice->deviceType() != AudioDevice::JACK_AUDIO)  // Jack not running
  {
    if(initMidiAlsa()) { /* fatal error */ }
  }
#endif
  if(initMidiJack()) { /* fatal error */ }  // always called, unconditionally
```
- **ALSA** is only initialized when explicitly enabled, forced via `-A`, or
  when JACK audio isn't running at all.
- **JACK** midi is *always* initialized, regardless of the above (this is
  what makes running both simultaneously possible, and is the origin of the
  cross-connection bugs this conversation dealt with).
- `-B` (added in this session, `main.cpp`) toggles `MusEGlobal::useSimplePortLabels`
  — unrelated to driver selection, only affects Jack midi label text style.

## 2. How port names and aliases are constructed

### ALSA
`MidiAlsaDevice` objects are named **directly after the discovered Alsa port
name** (e.g. `"Midi Through Port-0"`) — see `alsamidi.cpp` around line 1600
(`alsaScanMidiPorts()`'s add-device loop): `const QString dev_name(...k->name...)`.
There is no separate alias mechanism on the Alsa side; the device name *is*
the display name.

### JACK
Two independent name spaces exist, historically never reconciled — this was
the source of most of the confusion this conversation investigated:

- **MusE's own device/port name**: fixed at creation as `"jack-midi-" + N`
  (`jackmidi.cpp`, `createJackMidiDevice()`), suffixed `" in"`/`" out"` via
  `JACK_MIDI_IN_PORT_SUFFIX`/`_OUT_PORT_SUFFIX` in `setName()`/`open()`. Never
  derived from what the port connects to.
- **`Route::persistentJackPortName`**: the *stable, refindable* name of
  whatever this route is connected to — used purely for
  `jack_port_by_name()` lookups on reconnect, never shown to the user
  directly except as a raw fallback.
- **JACK Metadata "pretty-name"** (`http://jackaudio.org/metadata/pretty-name`,
  this session's addition): a cosmetic label MusE writes onto *its own* ports
  reflecting what they're connected to, e.g. `"Muse >> sys - midi through 1"`.
  Built by `buildFriendlyPortLabel()` (`jackmidi.cpp`), which normalizes the
  wildly inconsistent raw names a2jmidid/PipeWire's Midi-Bridge produce
  (leading bridge tag sometimes present, trailing `(capture)`/`(playback)`
  sometimes present) into a short `"<category> - <device> [<suffix>]"` form.
  Legacy `jack_port_set_alias()` (alias1/alias2) was abandoned mid-session —
  confirmed via `jack_lsp -A` to not be reliably persisted under PipeWire's
  Jack compatibility layer.

## 3. Where names/aliases are stored, and how to read them elsewhere

| What | Stored as | Where |
|---|---|---|
| Device's own name | `QString _name` (private) | `MidiDevice` base class; read via `dev->name()` |
| Reconnect target | `char persistentJackPortName[ROUTE_PERSISTENT_NAME_SIZE]` | `Route` struct (`route.h`) |
| Cached live port pointer | `void* jackPort` | `Route` struct — **may be null/stale**, always re-resolve via `findPort()` before trusting it (see `route.cpp` fixes) |
| Cosmetic connection label | JACK Metadata, key `JACK_METADATA_PRETTY_NAME` | **Not stored in MusE at all** — lives in the JACK/PipeWire server's own metadata store, per port UUID |

**Single access point for all of the above (display purposes):**
```cpp
virtual char* AudioDevice::portName(void* port, char* str, int str_size,
                                     int preferred_name_or_alias = -1);
```
(`audiodev.h` declares it, `JackAudioDevice::portName()` in `jack.cpp`
implements it.)
- `preferred_name_or_alias == -1` (default): returns the **real, refindable**
  port name — this is what must be used to build/store `persistentJackPortName`.
  Never returns the cosmetic pretty-name (fixed this session after it broke
  reconnection).
- `== 1` or `== 2`: returns the JACK Metadata pretty-name if set (for midi
  ports, run through `midiPortFriendlyName()`/`buildFriendlyPortLabel()` for
  the friendly categorized text); falls back to the legacy alias1/alias2 API,
  then to the canonical name.
- `== 0`: forces the canonical Jack port name, bypassing everything else.

Convenience wrappers on `Route` (`route.cpp`) — `Route::name(int)`,
`Route::name(char*, int, int)`, `Route::displayName(int)` — all forward to
`portName()`, re-resolving `jackPort` via `findPort(persistentJackPortName)`
first if the cached pointer is null.

Cross-file helpers (declared in `jackaudio.h`, shared between `jack.cpp` and
`jackmidi.cpp`): `isOwnBridgedMidiPort()`, `rawJackPortName()`,
`jackPortPrettyName()`, `midiPortFriendlyName()`.

## 4. Port scanning — where and when

### ALSA
`alsaScanMidiPorts()` (`alsamidi.cpp:1444`). Triggered by:
- `SND_SEQ_EVENT_PORT_START` / `PORT_EXIT` / `CLIENT_START` / `CLIENT_EXIT`
  ALSA sequencer announcements, handled in `alsaProcessMidiInput()`
  (`alsamidi.cpp` ~1749–1845): sets `atomicAlsaMidiScanPending` and posts a
  `'P'` message to the GUI thread (never scans inline from the ALSA callback).
- Explicit call from `song.cpp:4999`.

No periodic polling — purely event-driven.

### JACK
`enumerateJackMidiDevicesImpl()` (`jackmidi.cpp`, moved here from `helper.cpp`
this session) runs **once, at startup only** (`main.cpp` ~1700). There is
**no equivalent periodic full re-scan** for Jack — instead, newly-appeared
ports/connections are picked up incrementally by the graph-changed handling
described below (§5), not by re-running the enumeration.

## 5. Port auto-connection — triggers and rules

### ALSA
`alsaScanMidiPorts()`'s "devices to add" loop (`alsamidi.cpp` ~1585–1615):
matches a rediscovered Alsa port **by name** against any `MidiAlsaDevice`
whose address is currently `SND_SEQ_ADDRESS_UNKNOWN`, rebinds
`d->adr.client`/`port`, and calls `d->open()` → re-subscribes via
`snd_seq_port_subscribe_*`. Triggered by the same scan events as §4 — this is
also the mechanism behind "Alsa reconnects after manual disconnect": if the
remote client/port briefly disappears and reappears (common with
a2j/PipeWire's Alsa-seq bridge), the address resets to unknown and the next
scan re-subscribes by name match, independent of any manual disconnect you
did outside MusE.

### JACK
Three distinct mechanisms:

1. **`MidiJackDevice::open()`** (`jackmidi.cpp`) — at device creation / port
   assignment time (`populateMidiPorts()`, song load): iterates the device's
   persistent routes, resolves `findPort(persistentJackPortName)`, calls
   `jack_connect()` if found, then `setMidiConnectionAlias()`.
2. **`JackAudioDevice::graphChanged()` → `processGraphChanges()` →
   `processJackCallbackEvents()`** (`jack.cpp`) — triggered by JACK's own
   port-registration/connect/graph-order callbacks
   (`registration_callback`/`port_connect_callback`/`graph_callback`), which
   only push a lightweight event into a lock-free FIFO and signal the GUI
   thread (confirmed real-time safe — see fix report). For each known
   persistent route: if the remote port now exists and isn't connected, and
   `MusEGlobal::audio->isRunning()`, calls `jack_connect()`; if the remote
   port disappeared, clears the cached `jackPort` but keeps the route; uses
   `checkDisconnectCallback()`/`checkPortRegisterCallback()` to distinguish
   "disconnected then gone" from "disconnected then re-registered" via
   FIFO-event timing analysis.
3. **`checkNewRouteConnections()`** (`jack.cpp`) — adopts connections that
   already exist in the live graph (e.g. made externally via qjackctl) as
   persistent routes. Does **not** call `jack_connect()` itself, pure
   bookkeeping. Skips self-connections (MusE-to-MusE, incl. a2j/Midi-Bridge
   loops) via `isOwnBridgedMidiPort()`/`jack_port_is_mine()` — these are left
   alone if made by hand, but never remembered/auto-restored.

## 6. `populateMidiPorts()` — auto-assigning devices to `MidiPort` slots

`helper.cpp`, called from `main.cpp` ~line 1794, **only** when
`MusEGlobal::populateMidiPortsOnStart` is set and the project is genuinely new
(not loaded from an existing file). Fills `MusEGlobal::midiPorts[]` slots
from `MusEGlobal::midiDevices` (see §0) — this is the "middle layer" wiring,
separate from and unrelated to the Jack-graph auto-connect in §5.

Rules, in order:
1. If Jack audio is running: walk `midiDevices`, assign each **Jack midi**
   device to the next free `MidiPort` slot (`msgSetMidiDevice()`, which
   triggers `MidiDevice::open()` — the actual `jack_connect()` happens there,
   see §5.1). Devices default to 0 output channels, first-available-input
   device gets 1 input channel enabled, rest get 0.
   **Bug fixed this session**: this loop originally didn't check
   `dev->deviceType()`, so it could assign *Alsa* devices to ports even while
   preferring Jack — now explicitly filtered to `JACK_MIDI` only.
2. If Jack audio isn't running (`DUMMY_AUDIO`), **or** step 1 found zero Jack
   midi devices: same assignment loop, filtered to `ALSA_MIDI` devices
   instead. This lets a Jack-for-audio/Alsa-for-midi setup still get
   populated.
3. Stops early once all `MIDI_PORTS` slots are filled.

Note the comment in the code (`helper.cpp`) explicitly disables always
defaulting output channels to port #1 — intentional, so a freshly created
midi track can default to the *last* created port instead when there's no
existing default.

## 7. Related files

| File | General task |
|---|---|
| `mididev.cpp` | Driver bring-up (`initMidiDevices()`), generic `MidiDevice` base behavior |
| `midiport.cpp` | `MidiPort` class — the 0..MIDI_PORTS-1 slots tracks route through; `setMidiDevice()`/`open()` dispatch |
| `alsamidi.cpp` / `alsamidi.h` | `MidiAlsaDevice`, Alsa sequencer scanning (`alsaScanMidiPorts`), event I/O |
| `jackmidi.cpp` / `jackmidi.h` | `MidiJackDevice`, Jack midi port creation/naming/open/close, event I/O, label-building (`buildFriendlyPortLabel`, `midiPortFriendlyName`, `setMidiConnectionAlias`), `enumerateJackMidiDevicesImpl()` |
| `jack.cpp` / `jackaudio.h` | `JackAudioDevice` — generic (audio+midi) Jack port/route/graph handling, `portName()`, self-connection filtering, RT callback registration |
| `audiodev.h` | `AudioDevice` abstract base interface shared by Alsa/Jack/Dummy drivers |
| `route.cpp` / `route.h` | `Route` struct — persistent routing target, `name()`/`displayName()` |
| `helper.cpp` / `helper.h` | `populateMidiPorts()` (auto-fill new project's midi ports), misc; thin forwarder to `jackmidi.cpp`'s enumeration |
| `routepopup.cpp` / `routepopup.h` | `RoutePopupMenu` — the GUI routing popup (track in/out routing buttons), incl. the "Show aliases" First/Second/Name toggle |
| `midictrl.cpp` | Midi controller definitions (unrelated to routing, not touched this session) |
| `song.cpp` | Song-level triggers, incl. `alsaScanMidiPorts()` call at ~line 4999 |
| `main.cpp` | Startup sequencing, command-line switches (`-A`, `-B`, etc.) |
| `app.cpp` | `MusE::initMidiDevices()` — GUI-triggered *re-init* action (menu item), distinct from the one-time startup init in `mididev.cpp` |

## 8. Entry points for midi setup

**`main.cpp`** (startup sequence, in order):
- **~line 1611**: `MusECore::initMidiDevices();` — brings up Alsa/Jack midi
  drivers (see §1).
- **~line 1700**: `MusECore::enumerateJackMidiDevices();` — one-time Jack
  midi port discovery/pairing (thin forwarder into `jackmidi.cpp`).
- **~line 1794**: `MusECore::populateMidiPorts();` — conditional
  (`MusEGlobal::populateMidiPortsOnStart`, only for genuinely new
  projects/files) auto-assignment of discovered devices to the song's midi
  ports.

**`app.cpp`** (user-triggered, not startup):
- **~line 1545**: `MusE::initMidiDevices()` — GUI method wired to the
  "Init Midi Instruments" menu action (`app.cpp:939`); sends
  `MusEGlobal::audio->msgInitMidiDevices()`, a re-init distinct from the
  one-time startup path above.
- **~line 1536**: `MusE::resetMidiDevices()` — similarly wired to a menu
  action, sends `msgResetMidiDevices()`.

## 9. Real-time safety notes

Relevant when adding code anywhere in §4/§5's call chains. Verified by
tracing every call site (see the fix report for detail), not assumed:

- The three raw JACK-registered callbacks (`registration_callback`,
  `port_connect_callback`, `graph_callback` in `jack.cpp`) run in JACK's own
  notification thread. They do **only** lock-free-FIFO-push +
  atomic-flag-set + GUI message dispatch — nothing else is safe there.
- Everything else — `graphChanged()`, `processGraphChanges()`,
  `processJackCallbackEvents()`, `checkNewRouteConnections()`,
  `setMidiConnectionAlias()`, `jack_connect()`, `jack_set_property()`/
  `jack_get_property()`, all `QString` work — runs later, dispatched onto the
  **GUI/main thread**. None of it is RT-safe, none of it needs to be.
- The real RT audio callback (`JackAudioDevice::processAudio()`) and the
  actual Midi event I/O (`MidiJackDevice::processMidi()`,
  `collectMidiEvents()`, `queueEvent()`, `processEvent()`,
  `eventReceived()`) are a **completely separate path** and must stay
  allocation-/lock-/blocking-call-free. Don't add calls into §2/§3's naming
  machinery (all of it does string work and/or JACK server round-trips) from
  anywhere in that path.
- ALSA side: `alsaProcessMidiInput()`'s event handling also only sets an
  atomic flag and posts a GUI message (`'P'`) — the actual `alsaScanMidiPorts()`
  re-subscribe work happens later, same pattern as the Jack side.

## 10. `PendingOperationList` — thread-safe route mutation

Both `alsamidi.cpp` and `jack.cpp` build up route/device changes as a batch
rather than mutating shared lists directly, since those lists are also read
by the audio/midi processing thread:

```cpp
PendingOperationList operations;
operations.add(PendingOperationItem(route_list, r, PendingOperationItem::AddRouteNode));
operations.add(PendingOperationItem(dev->outRoutes(), dstRoute, PendingOperationItem::AddRouteNode));
...
if(!operations.empty())
  MusEGlobal::audio->msgExecutePendingOperations(operations);
```

Pattern: **GUI-thread code never mutates a `RouteList`/`MidiDeviceList`
in place.** It inspects current state (read-only), decides what *should*
change, and appends `PendingOperationItem` entries describing that change
(`AddRouteNode`, `ModifyRouteNode`, `DeleteRouteNode`, `AddMidiDevice`,
`ModifyMidiDeviceAddress`, `ModifyMidiDeviceFlags`, `UpdateSoloStates`, and
many non-midi ones for tracks/plugins/etc.). `msgExecutePendingOperations()`
hands the whole batch to the audio thread, which applies them atomically at
a safe point in its cycle, then returns. This is why so much of the
graph-changed code in §5 reads oddly indirect (e.g. "don't call jack_connect
here, add a route node instead, the graph callback will handle the actual
connect") — it's this queueing discipline, not an oversight.

Some operation types additionally get inspected *while building* a new batch
(see `checkNewRouteConnections()`, `jack.cpp`) — walking `operations` backward
to see if a route already has a pending `DeleteRouteNode`/`ModifyRouteNode`
queued, so as not to act on stale state twice in the same cycle.

## 11. Code examples

### List available Jack midi ports
```cpp
// true = midi ports only. aliases: -1 no preference, 0 canonical, 1/2 alias slot.
std::list<QString> in  = MusEGlobal::audioDevice->inputPorts(true);
std::list<QString> out = MusEGlobal::audioDevice->outputPorts(true);
for(const QString& name : out)
{
  void* port = MusEGlobal::audioDevice->findPort(name.toUtf8().constData());
  if(port)
    fprintf(stderr, "found midi output port: %s\n", name.toUtf8().constData());
}
```

### Get a port's friendly alias / display name
```cpp
// For display: preferred_name_or_alias = 1 (or MusEGlobal::config.preferredRouteNameOrAlias).
// For anything you intend to store/reconnect by later, use -1 instead - see §3.
char buf[ROUTE_PERSISTENT_NAME_SIZE];
MusEGlobal::audioDevice->portName(port, buf, ROUTE_PERSISTENT_NAME_SIZE, 1);
QString alias(buf);

// Or directly, for a midi port specifically (skips the legacy alias1/alias2
// fallback, always applies the friendly "sys -"/"blue -"/"<device> -" formatting):
QString friendly = MusECore::midiPortFriendlyName((jack_port_t*)port);
```

### Create / resolve a connection
```cpp
// By name (works before you have a live jack_port_t* for the far end):
bool ok = MusEGlobal::audioDevice->connect(our_port_name, remote_port_name);

// By live pointer (e.g. once you've resolved both ends):
bool ok2 = MusEGlobal::audioDevice->connect(our_port, remote_port);

// Disconnect is symmetric:
MusEGlobal::audioDevice->disconnect(our_port_name, remote_port_name);

// Check before connecting, if you need to avoid duplicate-connect errors:
if(MusEGlobal::audioDevice->portsCanConnect(our_port_name, remote_port_name))
  MusEGlobal::audioDevice->connect(our_port_name, remote_port_name);
```
Note: none of `connect()`/`disconnect()`/`findPort()`/`portName()` are
real-time safe (see §9) — only call these from GUI-thread-dispatched code,
never from the audio/midi processing path.
