# MusE MCP server

`musemcp.py` is an optional MCP server for AI clients. It uses MusE's existing
Python/Pyro bridge and communicates with MCP clients over stdio.

## Install

Use Python 3.10 or newer and install the optional dependencies into the Python
environment used to run the server. From the repository root:

```sh
python3 -m venv ~/.venvs/muse-mcp
~/.venvs/muse-mcp/bin/pip install -r src/share/pybridge/requirements-mcp.txt
```

When using the installed MusE files, replace the requirements path above with the
path to `share/muse/pybridge/requirements-mcp.txt` in the installation prefix.

## Start and connect

From MusE, open **Help → Connect AI Agent (MCP)** to check the requirements and
read-only connection, copy a client configuration, and choose the bridge startup
and permission profile. The dialog preselects `project_edit` for the generated client
configuration; choose a more restrictive profile if the agent should not modify the
project. It automatically prefers `~/.venvs/muse-mcp/bin/python` when available; otherwise
it falls back to `python3`, which may not have the MCP dependencies. Use **Check requirements**
to verify the selected interpreter. The saved startup setting takes effect the next time MusE
starts. Alternatively, start MusE with its Python bridge enabled:

```sh
muse -y
```

Then configure the MCP client to launch the installed or source-tree `musemcp.py`
using the same Python environment where the dependencies were installed. For example,
a client configuration can contain:

```json
{
  "mcpServers": {
    "muse": {
      "command": "/home/user/.venvs/muse-mcp/bin/python",
      "args": ["/path/to/pybridge/musemcp.py"],
      "env": {
        "MUSE_PYRO_NS_HOST": "127.0.0.1",
        "MUSE_MCP_PROFILE": "project_edit"
      }
    }
  }
}
```

The bridge starts a local Pyro nameserver when one is not already available. If MusE
was configured with a non-default nameserver, pass the same endpoint to the MCP
process with `MUSE_PYRO_NS_HOST` and `MUSE_PYRO_NS_PORT`. `MUSE_PYRO_TIMEOUT` optionally
sets the RPC timeout in seconds (default: `5`).

## Permission profiles

If `MUSE_MCP_PROFILE` is omitted, the server uses `read_only`, which allows song-state and
track-list reads only. The dialog preselects `project_edit` when generating a client
configuration. Set the profile to `transport` to additionally allow playback, stop, rewind,
and position changes. The `project_edit` profile also allows track mute, MIDI part creation
and editing, MIDI file imports, MIDI output routing, track parameter changes, instrument
track creation, audio effect management, and project file operations; these operations can
change or replace the currently open project. Unknown profiles are denied. The MCP server
enforces these limits even if a client tries to call a restricted tool directly.

## Available tools

- `muse_get_song_state` and `muse_list_tracks` read the open song.
- `muse_save_project` saves the current project to a `.med` file. It adds `.med` when the
  supplied path has no extension; replacing a different existing file requires
  `overwrite=true`.
- `muse_close_project` saves the current project before closing it, even when it is clean,
  and returns the actual `saved_to` path. For a never-saved project, pass `save_path`.
- `muse_create_project` creates a blank project from MusE's default template, saves it to
  the requested `.med` path, and leaves it open. It saves the current project first when
  dirty (`save_current=true` by default); use `save_current_path` for an untitled project.
  An existing destination is rejected unless `overwrite=true` is explicitly supplied.
- `muse_open_project` opens an existing `.med` file. It saves the current project first
  when dirty (`save_current=true` by default); use `save_current_path` for a new destination
  and `overwrite_current=true` only when intentionally replacing an existing file.
  If saving fails, the current project stays open. Opening and closing replace the active
  song, so verify the requested project path before calling them.
- `muse_start_playback`, `muse_stop_playback`, `muse_rewind`, and
  `muse_set_position` control transport. Positions are measured in ticks.
- `muse_set_track_mute` sets the mute state of an existing track.
- `muse_list_available_instruments` lists software instrument plugins MusE can load.
  `muse_add_instrument_track` creates the selected instrument track and assigns it to
  the first free MIDI output port; it fails if no port is available or the plugin cannot
  be instantiated. `muse_list_midi_outputs` and `muse_set_midi_track_output` allow MIDI
  tracks to be routed to an instrument and channel; `muse_get_midi_track_output` reads
  back the exact assigned port, channel, and output name when a device is attached.
- `muse_add_midi_track` adds a uniquely named MIDI track.
- `muse_list_midi_track_programs` lists named patches for the instrument routed to a MIDI
  track. `muse_set_midi_track_program` selects one of those patches immediately on the
  assigned output; MIDI file import still does not import program changes.
- `muse_create_midi_part`, `muse_list_midi_parts`, and `muse_update_midi_part` create,
  inspect, move, resize, or replace note events in MIDI parts. The create tool returns
  a part ID that can be used by the edit tool. Note offsets, durations, part positions,
  and lengths use MusE ticks.
- `muse_import_midi_file` reads a `.mid`/`.midi` file on the MCP host and creates a
  part on the selected MIDI track. It imports notes (including MIDI channel) and
  control-change events, converts the file's ticks-per-beat to the current song division,
  and accepts files up to 50 MiB, 4096 notes, and 8192 total note/controller events.
  Choose a source track index or combine all file tracks. It does not import tempo maps,
  program changes, SysEx, or other meta events.
- `muse_set_midi_track_parameter` sets transposition, velocity, delay, note length, or
  compression using MusE's ranges. `muse_list_midi_outputs` lists assigned MIDI devices
  and software synths; `muse_get_midi_track_output` reports a MIDI track's port and
  channel, while `muse_set_midi_track_output` connects it to a listed port and selects a
  channel from 0 to 15.
- `muse_list_available_track_effects` lists installed audio effects and their `plugin_id`;
  `muse_list_track_effects` inspects a track's rack. `muse_add_track_effect` installs
  an effect into an empty slot from 0 to 7, and `muse_set_track_effect_enabled` enables
  or bypasses an installed effect. `muse_list_track_effect_parameters` reports parameter
  names, current values, and accepted ranges; `muse_set_track_effect_parameter` updates
  a parameter only within that range. For a spacious, church-like sound, the agent can
  inspect installed effects, add a reverb, then adjust its room/decay/wet controls where
  the plugin exposes them. The available parameters depend on the selected plugin.

`muse_get_song_state` reports tempo as microseconds per quarter note. Track routing,
effects, and MIDI edits modify the currently open MusE project; use them only when those
changes are intended. MIDI file paths are resolved and read by the MCP server process.
Project paths are resolved on the MCP server host; existing `.med` files can be opened and
new projects can be created at a `.med` path. Project save, close, create, and open operations
require the `project_edit` profile.

## Security

The Pyro nameserver and daemon bind to `127.0.0.1` by default. They can instead bind
to a private IPv4 address (`10/8`, `172.16/12`, or `192.168/16`) selected in the
MusE dialog or configured on the command line. Public addresses, wildcard binds,
hostnames other than `localhost`, and IPv6 addresses are rejected. Binding to a
private network makes the unauthenticated Pyro bridge reachable by other devices on
that network; use it only on a trusted LAN. The MCP server itself uses stdio and
does not add another network listener.