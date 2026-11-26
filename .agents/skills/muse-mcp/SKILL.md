---
name: muse-mcp-workflow
description: Use MusE MCP tools to inspect and edit the currently open song, including MIDI parts, instrument routing, patches, and effects. Apply when an agent is asked to work on a MusE project through the MusE MCP server.
---

# MusE MCP workflow

Use this skill when operating MusE through the `muse` MCP server. The server communicates with the running MusE Python/Pyro bridge and acts on the currently open song; it is not a general-purpose MusE UI controller.

## Working rules

- Begin by calling `muse_get_song_state` and `muse_list_tracks`. Use their results as the source of truth for the active song, tempo, position, and track names. Do not assume a prior session's project is still open.
- If a requested action is denied, check the configured MCP permission profile rather than trying alternate paths to bypass it. `read_only` permits song and track reads, `transport` adds transport control, and `project_edit` allows project changes. `project_edit` is not a filesystem sandbox: it can read user-supplied MIDI/`.med` files and save `.med` files anywhere the MusE account can access, not only inside the active project folder. Use only paths the user requested or approved.
- Make only changes needed for the user's request. Use names and IDs returned by MCP tools; inspect existing tracks, parts, ports, programs, or effects before changing them.
- Use `muse_create_project` when asked to create a blank project: it loads MusE's default template, saves a `.med` file at the requested path, and leaves it open. It preserves dirty current work by saving first; an untitled project needs `save_current_path`.
- Positions, lengths, note offsets, and durations for MIDI parts/events are measured in MusE ticks. Use the active song's reported timing context; do not treat ticks as seconds or assume a fixed ticks-per-quarter value.

## MIDI and track workflow

- To add an instrument track, inspect `muse_list_available_instruments`, then call `muse_add_instrument_track` with a listed instrument. It uses the first free MIDI output port and can fail if no port is available or the plugin cannot load.
- For a MIDI track routed to an instrument, inspect `muse_list_midi_outputs` and use `muse_set_midi_track_output`; use channel numbers 0–15. Verify the actual port and channel with `muse_get_midi_track_output`. Use `muse_list_midi_track_programs` before selecting a patch with `muse_set_midi_track_program`.
- Create and edit parts with `muse_create_midi_part`, `muse_list_midi_parts`, and `muse_update_midi_part`. Keep the returned part ID for later edits. Read the current part before replacing or moving its contents, and avoid overwriting existing material unless requested.
- Before importing MIDI, verify the host-side file path and intended source track (or that all file tracks should be combined). Import supports notes and control-change events, converts the file's ticks-per-beat to the song division, and has limits of 50 MiB, 4096 notes, and 8192 total note/controller events. It does not import tempo maps, program changes, SysEx, or other meta events; account for this when planning the result.
- Track parameters and effects are plugin-dependent. Inspect available effects and the target track's rack/parameter ranges before changing them. For a requested spacious or church-like sound, use an available reverb and tune only controls actually exposed by that plugin; do not promise a specific acoustic result without listening/confirmation.

## Verify and persist

- After edits, re-read song state, track routing, parts, or effect parameters relevant to the request. Report what succeeded and any tool or plugin limitations; do not claim that audio was auditioned unless it was.
- `muse_save_project`, `muse_close_project`, `muse_create_project`, and `muse_open_project` can persist, close, or replace the active song. Call them only when the user explicitly asks for saving, closing, creating, or opening a project. Before creating, opening, or overwriting, confirm the exact path and any overwrite intent; preserve the current project if saving it fails. `muse_close_project` saves before it closes and reports the saved path.
- In the final response, summarize the concrete changes and verification, and state clearly whether the project was saved. Never imply that an edit is saved when only the in-memory song was changed.

For exact installation, tool descriptions, permission profiles, and security details, consult the repository's `README.mcp.md`.
