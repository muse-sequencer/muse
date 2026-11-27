#!/usr/bin/env python3
"""MCP server exposing selected MusE controls through the Python bridge."""

from __future__ import annotations

import json
import math
import os
import sys
import time
from collections import defaultdict, deque
from pathlib import Path
from typing import Annotated, Any, Literal

import Pyro5.api as pyro
from Pyro5.errors import PyroError
from mcp.server.fastmcp import FastMCP
from pydantic import BaseModel, Field
from musepyroconfig import normalize_bind_host


mcp = FastMCP("muse_mcp")
MAX_TICK = 2_147_483_647
MAX_MIDI_FILE_BYTES = 50 * 1024 * 1024
MAX_IMPORTED_NOTES = 4096
MAX_IMPORTED_EVENTS = 8192
PROJECT_OPERATION_METHODS = frozenset({
    "saveProjectFile", "closeProjectFile", "createProjectFile", "openProjectFile"
})
_PROFILE_LEVELS = {"read_only": 0, "transport": 1, "project_edit": 2}
_MIDI_PARAMETER_RANGES = {
    "transposition": (-127, 127),
    "velocity": (-127, 127),
    "delay": (-1000, 1000),
    "length": (25, 200),
    "compression": (25, 200),
}


class MidiNote(BaseModel):
    """A note event relative to the start of its MIDI part."""

    pitch: Annotated[int, Field(ge=0, le=127, description="MIDI note number, from 0 to 127")]
    velocity: Annotated[int, Field(ge=1, le=127, description="Note-on velocity, from 1 to 127")]
    tick: Annotated[int, Field(ge=0, le=MAX_TICK, description="Note start offset in ticks within the part")]
    length_ticks: Annotated[int, Field(ge=1, le=MAX_TICK, description="Note duration in ticks")]
    channel: Annotated[int, Field(ge=0, le=15, description="MIDI channel, from 0 to 15")] = 0


TrackName = Annotated[str, Field(min_length=1, max_length=128, description="Exact MusE track name")]
ProjectPath = Annotated[str, Field(min_length=1, max_length=4096, description="Path to a MusE .med project file")]
Tick = Annotated[int, Field(ge=0, le=MAX_TICK, description="Position measured in MusE ticks")]
PartId = Annotated[str, Field(min_length=1, max_length=64, description="Unique part ID returned by MusE")]


def _require_track(track_name: str, *, midi_only: bool = False) -> None:
    if track_name not in _muse_call("getTrackNames"):
        raise ValueError(f"No track named {track_name!r} exists in the open song")
    if midi_only and _muse_call("getParts", track_name) is None:
        raise ValueError(f"Track {track_name!r} is not a MIDI track")


def _require_muse_success(method: str, *args: Any, failure_message: str | None = None) -> None:
    result = _muse_call(method, *args)
    if result is not True:
        message = failure_message or f"MusE did not confirm {method}"
        raise RuntimeError(f"{message} (bridge returned {result!r})")


def _midi_note_events(notes: list[MidiNote]) -> list[dict[str, Any]]:
    return [
        {
            "data": [note.pitch, note.velocity, getattr(note, "channel", 0)],
            "tick": note.tick,
            "type": "note",
            "len": note.length_ticks,
        }
        for note in notes
    ]


def _validate_midi_file_path(path: str) -> Path:
    midi_path = Path(path).expanduser()
    if midi_path.suffix.lower() not in {".mid", ".midi"}:
        raise ValueError("MIDI file path must end in .mid or .midi")
    try:
        midi_path = midi_path.resolve(strict=True)
        if not midi_path.is_file():
            raise ValueError("MIDI file path must refer to a regular file")
        if midi_path.stat().st_size > MAX_MIDI_FILE_BYTES:
            raise ValueError(f"MIDI file exceeds the {MAX_MIDI_FILE_BYTES}-byte size limit")
    except OSError as exc:
        raise ValueError(f"MIDI file cannot be read: {exc}") from exc
    return midi_path


def _validate_project_save_path(path: str) -> Path:
    if not path or not path.strip() or len(path) > 4096:
        raise ValueError("Project save path must contain between 1 and 4096 characters")
    project_path = Path(path).expanduser()
    if not project_path.suffix:
        project_path = project_path.with_suffix(".med")
    elif project_path.suffix.lower() != ".med":
        raise ValueError("MusE project save path must end in .med")
    try:
        return project_path.resolve(strict=False)
    except OSError as exc:
        raise ValueError(f"Project save path is invalid: {exc}") from exc


def _validate_project_open_path(path: str) -> Path:
    if not path or not path.strip() or len(path) > 4096:
        raise ValueError("Project path must contain between 1 and 4096 characters")
    project_path = Path(path).expanduser()
    if project_path.suffix.lower() != ".med":
        raise ValueError("Project path must end in .med")
    try:
        project_path = project_path.resolve(strict=True)
        if not project_path.is_file():
            raise ValueError("Project path must refer to a readable MusE .med project")
        with project_path.open("rb"):
            pass
    except OSError as exc:
        raise ValueError(f"Project file must be a readable MusE .med project: {exc}") from exc
    return project_path


def _effect_identifier(effect: dict[str, Any]) -> str:
    uri = str(effect.get("uri", ""))
    if uri:
        return uri
    return json.dumps([str(effect.get("file", "")), str(effect.get("label", ""))], separators=(",", ":"))


def _instrument_identifier(instrument: dict[str, Any]) -> str:
    uri = str(instrument.get("uri", ""))
    if uri:
        return uri
    return json.dumps(
        [int(instrument.get("type", -1)), str(instrument.get("file", "")), str(instrument.get("label", ""))],
        separators=(",", ":"),
    )


def _wait_for_track(track_name: str, timeout_seconds: float = 15.0) -> bool:
    deadline = time.monotonic() + timeout_seconds
    while True:
        if track_name in _muse_call("getTrackNames"):
            return True
        if time.monotonic() >= deadline:
            return False
        time.sleep(0.05)


def _read_midi_file(
    path: Path, source_track_index: int | None, song_division: int
) -> tuple[list[MidiNote], list[dict[str, Any]], int]:
    try:
        import mido

        midi_file = mido.MidiFile(str(path))
    except Exception as exc:
        raise ValueError(f"Could not parse MIDI file: {exc}") from exc
    if midi_file.ticks_per_beat <= 0 or song_division <= 0:
        raise ValueError("MIDI ticks-per-beat and MusE song division must be positive")
    if source_track_index is not None and source_track_index >= len(midi_file.tracks):
        raise ValueError(f"MIDI file has no source track at index {source_track_index}")

    tracks = (
        [(source_track_index, midi_file.tracks[source_track_index])]
        if source_track_index is not None
        else list(enumerate(midi_file.tracks))
    )
    notes: list[MidiNote] = []
    controller_events: list[dict[str, Any]] = []
    full_length = 0
    source_ticks_per_beat = midi_file.ticks_per_beat
    for _, midi_track in tracks:
        active_notes: dict[tuple[int, int], deque[tuple[int, int]]] = defaultdict(deque)
        absolute_tick = 0
        for message in midi_track:
            delta = int(message.time)
            if delta < 0:
                raise ValueError("MIDI messages cannot have negative delta times")
            absolute_tick += delta
            if message.type == "note_on" and int(message.velocity) > 0:
                active_notes[(int(message.channel), int(message.note))].append(
                    (absolute_tick, int(message.velocity))
                )
            elif message.type == "note_off" or (message.type == "note_on" and int(message.velocity) == 0):
                key = (int(message.channel), int(message.note))
                if not active_notes[key]:
                    continue
                note_tick, velocity = active_notes[key].popleft()
                start_tick = (note_tick * song_division + source_ticks_per_beat // 2) // source_ticks_per_beat
                end_tick = (absolute_tick * song_division + source_ticks_per_beat // 2) // source_ticks_per_beat
                end_tick = max(end_tick, start_tick + 1)
                notes.append(MidiNote(
                    pitch=int(message.note),
                    velocity=velocity,
                    tick=start_tick,
                    length_ticks=end_tick - start_tick,
                    channel=int(message.channel),
                ))
                full_length = max(full_length, end_tick)
                if len(notes) > MAX_IMPORTED_NOTES:
                    raise ValueError(f"MIDI import is limited to {MAX_IMPORTED_NOTES} notes per part")
            elif message.type == "control_change":
                controller = message.control
                value = message.value
                channel = message.channel
                if any(type(field) is not int for field in (controller, value, channel)):
                    raise ValueError("MIDI controller number, value, and channel must be integers")
                if not 0 <= controller <= 127 or not 0 <= value <= 127 or not 0 <= channel <= 15:
                    raise ValueError("MIDI controller number, value, or channel is out of range")
                controller_tick = (
                    absolute_tick * song_division + source_ticks_per_beat // 2
                ) // source_ticks_per_beat
                controller_events.append({
                    "data": [controller, value, channel],
                    "tick": controller_tick,
                    "type": "ctrl",
                })
                if len(notes) + len(controller_events) > MAX_IMPORTED_EVENTS:
                    raise ValueError(f"MIDI import is limited to {MAX_IMPORTED_EVENTS} note and controller events")
        converted_length = (absolute_tick * song_division + source_ticks_per_beat // 2) // source_ticks_per_beat
        full_length = max(full_length, converted_length)

    if not notes and not controller_events:
        raise ValueError("MIDI file contains no complete note or controller events on the selected source track(s)")
    return notes, controller_events, max(1, full_length)


def _nameserver_options() -> tuple[str, int | None, float]:
    host = normalize_bind_host(os.environ.get("MUSE_PYRO_NS_HOST"))
    port_value = os.environ.get("MUSE_PYRO_NS_PORT", "").strip()
    timeout_value = os.environ.get("MUSE_PYRO_TIMEOUT", "5").strip()

    try:
        port = int(port_value) if port_value else None
        timeout = float(timeout_value)
    except ValueError as exc:
        raise RuntimeError("MUSE_PYRO_NS_PORT must be an integer and MUSE_PYRO_TIMEOUT a number") from exc

    if port is not None and not 1 <= port <= 65535:
        raise RuntimeError("MUSE_PYRO_NS_PORT must be between 1 and 65535")
    if timeout <= 0:
        raise RuntimeError("MUSE_PYRO_TIMEOUT must be greater than zero")

    return host, port, timeout


def _require_profile(required_profile: str) -> None:
    profile = os.environ.get("MUSE_MCP_PROFILE", "read_only").strip().lower()
    if profile not in _PROFILE_LEVELS:
        raise PermissionError("Unknown MUSE_MCP_PROFILE; access is denied")
    if _PROFILE_LEVELS[profile] < _PROFILE_LEVELS[required_profile]:
        raise PermissionError(
            f"This tool requires the {required_profile!r} MCP permission profile"
        )


def _muse_call(method: str, *args: Any) -> Any:
    """Call one exposed method on the running MusE Python bridge."""
    nameserver = None
    proxy = None
    try:
        host, port, timeout = _nameserver_options()
        nameserver = pyro.locate_ns(host=host, port=port)
        uri = nameserver.lookup("muse")
        proxy = pyro.Proxy(uri)
        # A project transition cannot be safely interrupted after MusE begins
        # clearing/loading the song; wait for its GUI-thread operation to finish.
        proxy._pyroTimeout = None if method in PROJECT_OPERATION_METHODS else timeout
        return getattr(proxy, method)(*args)
    except (PyroError, OSError) as exc:
        raise RuntimeError(
            "Cannot reach the MusE Python bridge. Start MusE with Python control enabled "
            "(muse -y or the saved startup setting) and check the Pyro nameserver host and port."
        ) from exc
    finally:
        if proxy is not None:
            proxy._pyroRelease()
        if nameserver is not None:
            nameserver._pyroRelease()


@mcp.tool(
    name="muse_get_song_state",
    annotations={
        "title": "Get MusE Song State",
        "readOnlyHint": True,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_get_song_state() -> dict[str, Any]:
    """Get current song position, tempo, locators, loop state, and track names.

    Tempo is returned in microseconds per quarter note; all positions and lengths
    are MusE ticks. This tool only reads the currently open song.
    """
    _require_profile("read_only")
    position = int(_muse_call("getCPos"))
    return {
        "position_ticks": position,
        "song_length_ticks": int(_muse_call("getSongLen")),
        "ticks_per_quarter_note": int(_muse_call("getDivision")),
        "tempo_us_per_quarter_note": int(_muse_call("getTempo", position)),
        "left_locator_ticks": int(_muse_call("getLPos")),
        "right_locator_ticks": int(_muse_call("getRPos")),
        "loop_enabled": bool(_muse_call("getLoop")),
        "tracks": list(_muse_call("getTrackNames")),
    }


@mcp.tool(
    name="muse_save_project",
    annotations={
        "title": "Save MusE Project",
        "readOnlyHint": False,
        "destructiveHint": True,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_save_project(path: ProjectPath, overwrite: bool = False) -> dict[str, Any]:
    """Save the current MusE project to a .med file; an omitted extension becomes .med."""
    _require_profile("project_edit")
    project_path = _validate_project_save_path(path)
    _require_muse_success(
        "saveProjectFile", str(project_path), overwrite,
        failure_message=(
            "MusE could not save the project; choose a writable .med path, enable overwrite only "
            "when replacing that file is intended, and ensure no project load is in progress"
        ),
    )
    return {"file_path": str(project_path), "status": "saved"}


@mcp.tool(
    name="muse_close_project",
    annotations={
        "title": "Save and Close MusE Project",
        "readOnlyHint": False,
        "destructiveHint": True,
        "idempotentHint": False,
        "openWorldHint": False,
    },
)
def muse_close_project(save_path: ProjectPath | None = None, overwrite: bool = False) -> dict[str, Any]:
    """Save the current project before closing it; save_path is required for an untitled project."""
    _require_profile("project_edit")
    normalized_path = str(_validate_project_save_path(save_path)) if save_path else ""
    saved_path = _muse_call("closeProjectFile", normalized_path, overwrite)
    if not isinstance(saved_path, str) or not saved_path:
        raise RuntimeError(
            "MusE could not save and close the project. If it has never been saved, provide save_path; "
            "set overwrite=True only when replacing an existing file is intended "
            f"(bridge returned {saved_path!r})"
        )
    return {"status": "closed", "saved_to": saved_path}


@mcp.tool(
    name="muse_create_project",
    annotations={
        "title": "Create and Open MusE Project",
        "readOnlyHint": False,
        "destructiveHint": True,
        "idempotentHint": False,
        "openWorldHint": False,
    },
)
def muse_create_project(
    path: ProjectPath,
    save_current: bool = True,
    save_current_path: ProjectPath | None = None,
    overwrite: bool = False,
) -> dict[str, Any]:
    """Create and open a blank project at path, saving the current project first when dirty."""
    _require_profile("project_edit")
    project_path = _validate_project_save_path(path)
    if project_path.exists() and not overwrite:
        raise ValueError("Project file already exists; set overwrite=True only when replacing it is intended")
    normalized_save_path = str(_validate_project_save_path(save_current_path)) if save_current_path else ""
    _require_muse_success(
        "createProjectFile", str(project_path), save_current, normalized_save_path, overwrite,
        failure_message=(
            "MusE could not create the project. Save the current dirty project first, provide "
            "save_current_path for an untitled project, and only overwrite an existing destination "
            "when intended"
        ),
    )
    return {"file_path": str(project_path), "status": "created"}


@mcp.tool(
    name="muse_open_project",
    annotations={
        "title": "Save Current and Open MusE Project",
        "readOnlyHint": False,
        "destructiveHint": True,
        "idempotentHint": False,
        "openWorldHint": False,
    },
)
def muse_open_project(
    path: ProjectPath,
    save_current: bool = True,
    save_current_path: ProjectPath | None = None,
    overwrite_current: bool = False,
) -> dict[str, Any]:
    """Open an existing .med project, saving the current project first when it has unsaved changes."""
    _require_profile("project_edit")
    project_path = _validate_project_open_path(path)
    normalized_save_path = str(_validate_project_save_path(save_current_path)) if save_current_path else ""
    _require_muse_success(
        "openProjectFile", str(project_path), save_current, normalized_save_path, overwrite_current,
        failure_message=(
            "MusE could not open the project. Verify the .med file, wait for any active load to finish, "
            "and save the current project's unsaved changes before opening another file"
        ),
    )
    return {"file_path": str(project_path), "status": "opened"}


@mcp.tool(
    name="muse_list_tracks",
    annotations={
        "title": "List MusE Tracks",
        "readOnlyHint": True,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_list_tracks() -> list[str]:
    """List the names of tracks in the currently open MusE song."""
    _require_profile("read_only")
    return list(_muse_call("getTrackNames"))


@mcp.tool(
    name="muse_start_playback",
    annotations={
        "title": "Start MusE Playback",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_start_playback() -> str:
    """Start playback from the current song position."""
    _require_profile("transport")
    _require_muse_success("startPlay")
    return "Playback start accepted by MusE."


@mcp.tool(
    name="muse_stop_playback",
    annotations={
        "title": "Stop MusE Playback",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_stop_playback() -> str:
    """Stop MusE playback."""
    _require_profile("transport")
    _require_muse_success("stopPlay")
    return "Playback stop accepted by MusE."


@mcp.tool(
    name="muse_rewind",
    annotations={
        "title": "Rewind MusE Song",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_rewind() -> str:
    """Rewind the song transport to the start."""
    _require_profile("transport")
    _require_muse_success("rewindStart")
    return "Rewind accepted by MusE."


@mcp.tool(
    name="muse_set_position",
    annotations={
        "title": "Set MusE Song Position",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_set_position(tick: Tick) -> dict[str, Any]:
    """Move the song transport to an absolute position in MusE ticks."""
    _require_profile("transport")
    if not 0 <= tick <= MAX_TICK:
        raise ValueError(f"tick must be between zero and {MAX_TICK}")
    _require_muse_success("setCPos", tick)
    return {"position_ticks": tick, "status": "updated"}


@mcp.tool(
    name="muse_set_track_mute",
    annotations={
        "title": "Set MusE Track Mute",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_set_track_mute(track_name: TrackName, muted: bool) -> dict[str, Any]:
    """Set mute state for the track with this exact name."""
    _require_profile("project_edit")
    if track_name not in _muse_call("getTrackNames"):
        raise ValueError(f"No track named {track_name!r} exists in the open song")
    _require_muse_success("setMute", track_name, muted)
    return {"track_name": track_name, "muted": muted, "status": "updated"}


@mcp.tool(
    name="muse_add_midi_track",
    annotations={
        "title": "Add MusE MIDI Track",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": False,
    },
)
def muse_add_midi_track(track_name: TrackName) -> dict[str, str]:
    """Add a named MIDI track to the current song; the name must be unique."""
    _require_profile("project_edit")
    if not track_name.strip():
        raise ValueError("track_name cannot be empty or whitespace")
    if track_name in _muse_call("getTrackNames"):
        raise ValueError(f"A track named {track_name!r} already exists")
    _require_muse_success(
        "addMidiTrack", track_name,
        failure_message=f"MusE rejected the MIDI track {track_name!r}",
    )
    if not _wait_for_track(track_name):
        raise RuntimeError(f"MusE did not add the MIDI track {track_name!r} before the timeout")
    return {"track_name": track_name, "track_type": "midi"}


@mcp.tool(
    name="muse_list_available_instruments",
    annotations={
        "title": "List Available Instruments",
        "readOnlyHint": True,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_list_available_instruments() -> list[dict[str, Any]]:
    """List installed software instrument plugins that MusE can add as tracks."""
    _require_profile("read_only")
    instruments = list(_muse_call("getAvailableInstruments"))
    return [{**instrument, "plugin_id": _instrument_identifier(instrument)} for instrument in instruments]


@mcp.tool(
    name="muse_add_instrument_track",
    annotations={
        "title": "Add Software Instrument Track",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": False,
    },
)
def muse_add_instrument_track(
    plugin_id: Annotated[str, Field(min_length=1, max_length=4096)],
    track_name: TrackName,
) -> dict[str, Any]:
    """Add an installed instrument and return the MIDI output port assigned by MusE."""
    _require_profile("project_edit")
    if not track_name.strip():
        raise ValueError("track_name cannot be empty or whitespace")
    if track_name in _muse_call("getTrackNames"):
        raise ValueError(f"A track named {track_name!r} already exists")
    instruments = list(_muse_call("getAvailableInstruments"))
    instrument = next(
        (item for item in instruments if _instrument_identifier(item) == plugin_id),
        None,
    )
    if instrument is None:
        raise ValueError("Unknown instrument plugin_id; list available instruments and use its plugin_id")
    midi_output_port = _muse_call(
        "addSynthTrack",
        int(instrument["type"]),
        str(instrument.get("file", "")),
        str(instrument.get("uri", "")),
        str(instrument.get("label", "")),
        track_name,
    )
    if type(midi_output_port) is not int or midi_output_port < 0:
        raise RuntimeError(
            "MusE did not confirm instrument creation and MIDI output allocation "
            f"(bridge returned {midi_output_port!r})"
        )
    if not _wait_for_track(track_name):
        raise RuntimeError(f"MusE did not create instrument track {track_name!r} before the timeout")
    return {
        "track_name": track_name,
        "instrument": str(instrument.get("name", "")),
        "plugin_id": plugin_id,
        "midi_output_port": midi_output_port,
        "status": "created",
    }


@mcp.tool(
    name="muse_create_midi_part",
    annotations={
        "title": "Create MusE MIDI Part",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": False,
    },
)
def muse_create_midi_part(
    track_name: TrackName,
    start_tick: Tick,
    length_ticks: Annotated[int, Field(ge=1, le=MAX_TICK, description="Part duration in ticks")],
    notes: Annotated[list[MidiNote], Field(min_length=1, max_length=512)],
) -> dict[str, Any]:
    """Create a MIDI part containing notes on an existing MIDI track.

    Note ticks are relative to the part start. Returns the part ID needed to edit
    or move this part later.
    """
    _require_profile("project_edit")
    if not track_name.strip():
        raise ValueError("track_name cannot be empty or whitespace")
    if start_tick + length_ticks > MAX_TICK:
        raise ValueError(f"The part must end at or before tick {MAX_TICK}")
    if any(note.tick + note.length_ticks > length_ticks for note in notes):
        raise ValueError("Each note must end within the part length")
    _require_track(track_name, midi_only=True)

    part_id = _muse_call("createPart", track_name, start_tick, length_ticks, {
        "tick": 0,
        "events": _midi_note_events(notes),
    })
    if not part_id:
        raise RuntimeError(f"MusE did not create a MIDI part on {track_name!r}")
    return {
        "track_name": track_name,
        "part_id": str(part_id),
        "start_tick": start_tick,
        "length_ticks": length_ticks,
        "note_count": len(notes),
        "status": "created",
    }


@mcp.tool(
    name="muse_list_midi_parts",
    annotations={
        "title": "List MIDI Parts",
        "readOnlyHint": True,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_list_midi_parts(track_name: TrackName) -> list[dict[str, Any]]:
    """List MIDI parts on a track, including part IDs, positions, lengths, and events."""
    _require_profile("read_only")
    _require_track(track_name)
    parts = _muse_call("getParts", track_name)
    if parts is None:
        raise ValueError(f"Track {track_name!r} is not a MIDI track")
    return list(parts)


@mcp.tool(
    name="muse_update_midi_part",
    annotations={
        "title": "Edit or Move MIDI Part",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_update_midi_part(
    track_name: TrackName,
    part_id: PartId,
    start_tick: Tick | None = None,
    length_ticks: Annotated[int | None, Field(ge=1, le=MAX_TICK)] = None,
    notes: Annotated[list[MidiNote] | None, Field(max_length=512)] = None,
) -> dict[str, Any]:
    """Move, resize, or replace the notes of an existing MIDI part.

    Unspecified position, length, and notes keep their current values. Supplying
    notes replaces the note events while preserving existing controller events.
    """
    _require_profile("project_edit")
    _require_track(track_name)
    if start_tick is None and length_ticks is None and notes is None:
        raise ValueError("Provide at least one of start_tick, length_ticks, or notes")
    parts = _muse_call("getParts", track_name)
    if parts is None:
        raise ValueError(f"Track {track_name!r} is not a MIDI track")
    existing = next((part for part in parts if str(part.get("id")) == part_id), None)
    if existing is None:
        raise ValueError(f"No MIDI part with ID {part_id!r} exists on {track_name!r}")

    new_start = int(existing["tick"]) if start_tick is None else start_tick
    new_length = int(existing["len"]) if length_ticks is None else length_ticks
    if new_start + new_length > MAX_TICK:
        raise ValueError(f"The part must end at or before tick {MAX_TICK}")
    existing_events = list(existing.get("events", []))
    controller_events = [event for event in existing_events if event.get("type") == "ctrl"]
    note_events = [event for event in existing_events if event.get("type") == "note"]
    if notes is not None:
        if any(note.tick + note.length_ticks > new_length for note in notes):
            raise ValueError("Each note must end within the part length")
        note_events = _midi_note_events(notes)
    elif any(int(event["tick"]) + int(event["len"]) > new_length for event in note_events):
        raise ValueError("The new part length would cut off existing notes; provide replacement notes or a longer length")

    updated = {
        "id": part_id,
        "tick": new_start,
        "len": new_length,
        "events": controller_events + note_events,
    }
    if not _muse_call("modifyPart", updated):
        raise RuntimeError(f"MusE could not update MIDI part {part_id!r}")
    return {
        "track_name": track_name,
        "part_id": part_id,
        "start_tick": new_start,
        "length_ticks": new_length,
        "note_count": len(note_events),
        "status": "updated",
    }


@mcp.tool(
    name="muse_import_midi_file",
    annotations={
        "title": "Import MIDI File to Track",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": False,
    },
)
def muse_import_midi_file(
    track_name: TrackName,
    file_path: Annotated[str, Field(min_length=1, max_length=4096)],
    start_tick: Tick = 0,
    source_track_index: Annotated[int | None, Field(ge=0, le=65535)] = None,
) -> dict[str, Any]:
    """Read a MIDI file and import its note content as a new part on a MIDI track.

    If source_track_index is omitted, notes from all MIDI file tracks are combined.
    Ticks are converted to the current MusE song division; at most 50 MiB and 4096
    notes are accepted. The file is read by this MCP process, not by MusE.
    """
    _require_profile("project_edit")
    _require_track(track_name, midi_only=True)
    midi_path = _validate_midi_file_path(file_path)
    division = int(_muse_call("getDivision"))
    notes, controller_events, length_ticks = _read_midi_file(midi_path, source_track_index, division)
    if start_tick + length_ticks > MAX_TICK:
        raise ValueError(f"Imported part must end at or before tick {MAX_TICK}")
    part_id = _muse_call("createPart", track_name, start_tick, length_ticks, {
        "tick": 0,
        "events": sorted(_midi_note_events(notes) + controller_events, key=lambda event: event["tick"]),
    })
    if not part_id:
        raise RuntimeError(f"MusE did not import the MIDI file to {track_name!r}")
    return {
        "track_name": track_name,
        "part_id": str(part_id),
        "source_file": str(midi_path),
        "source_track_index": source_track_index,
        "start_tick": start_tick,
        "length_ticks": length_ticks,
        "note_count": len(notes),
        "controller_event_count": len(controller_events),
        "status": "imported",
    }


@mcp.tool(
    name="muse_set_midi_track_parameter",
    annotations={
        "title": "Set MIDI Track Parameter",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_set_midi_track_parameter(
    track_name: TrackName,
    parameter: Literal["transposition", "velocity", "delay", "length", "compression"],
    value: int,
) -> dict[str, Any]:
    """Set a MIDI track property within MusE's supported range."""
    _require_profile("project_edit")
    _require_track(track_name, midi_only=True)
    minimum, maximum = _MIDI_PARAMETER_RANGES[parameter]
    if not minimum <= value <= maximum:
        raise ValueError(f"{parameter} must be between {minimum} and {maximum}")
    bridge_parameter = "len" if parameter == "length" else parameter
    _require_muse_success(
        "setMidiTrackParameter", track_name, bridge_parameter, value,
        failure_message=f"MusE could not set {parameter!r} on {track_name!r}",
    )
    return {"track_name": track_name, "parameter": parameter, "value": value, "status": "updated"}


@mcp.tool(
    name="muse_list_midi_outputs",
    annotations={
        "title": "List MIDI Outputs",
        "readOnlyHint": True,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_list_midi_outputs() -> list[dict[str, Any]]:
    """List MIDI output ports with an assigned hardware device or software synth."""
    _require_profile("read_only")
    return list(_muse_call("getMidiOutputs"))


@mcp.tool(
    name="muse_get_midi_track_output",
    annotations={
        "title": "Get MIDI Track Output",
        "readOnlyHint": True,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_get_midi_track_output(track_name: TrackName) -> dict[str, Any]:
    """Read the MIDI output port and channel assigned to a MIDI track."""
    _require_profile("read_only")
    _require_track(track_name, midi_only=True)
    routing = _muse_call("getMidiTrackOutput", track_name)
    if not isinstance(routing, dict) or "port" not in routing or "channel" not in routing:
        raise RuntimeError(f"MusE could not read the MIDI output assigned to {track_name!r}")
    port_index = int(routing["port"])
    channel = int(routing["channel"])
    output = next(
        (item for item in _muse_call("getMidiOutputs") if int(item["port"]) == port_index),
        None,
    )
    return {
        "track_name": track_name,
        "port_index": port_index,
        "output_name": str(output["name"]) if output is not None else None,
        "channel": channel,
        "output_assigned": output is not None,
    }


@mcp.tool(
    name="muse_set_midi_track_output",
    annotations={
        "title": "Connect MIDI Track to Instrument Output",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_set_midi_track_output(
    track_name: TrackName,
    port_index: Annotated[int, Field(ge=0, le=255)],
    channel: Annotated[int, Field(ge=0, le=15)] = 0,
) -> dict[str, Any]:
    """Route a MIDI track to an assigned device or synth output and MIDI channel."""
    _require_profile("project_edit")
    _require_track(track_name, midi_only=True)
    outputs = list(_muse_call("getMidiOutputs"))
    output = next((item for item in outputs if int(item["port"]) == port_index), None)
    if output is None:
        raise ValueError(f"MIDI output port {port_index} is not assigned to a device or synth")
    _require_muse_success(
        "setMidiTrackOutput", track_name, port_index, channel,
        failure_message=f"MusE could not route {track_name!r} to MIDI output {port_index}",
    )
    return {
        "track_name": track_name,
        "port_index": port_index,
        "output_name": str(output["name"]),
        "channel": channel,
        "status": "updated",
    }


@mcp.tool(
    name="muse_list_midi_track_programs",
    annotations={
        "title": "List MIDI Track Programs",
        "readOnlyHint": True,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_list_midi_track_programs(track_name: TrackName) -> list[dict[str, Any]]:
    """List named bank/program patches supplied by the MIDI track's assigned instrument."""
    _require_profile("read_only")
    _require_track(track_name, midi_only=True)
    programs = _muse_call("getMidiTrackPrograms", track_name)
    if programs is None:
        raise ValueError(f"Track {track_name!r} has no assigned output instrument with a patch list")
    return list(programs)


@mcp.tool(
    name="muse_set_midi_track_program",
    annotations={
        "title": "Select MIDI Track Program",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_set_midi_track_program(
    track_name: TrackName,
    patch_code: Annotated[int, Field(ge=0, le=0xFFFFFF)],
) -> dict[str, Any]:
    """Select a listed bank/program patch on the MIDI output assigned to a track."""
    _require_profile("project_edit")
    _require_track(track_name, midi_only=True)
    programs = _muse_call("getMidiTrackPrograms", track_name)
    if programs is None:
        raise ValueError(f"Track {track_name!r} has no assigned output instrument with a patch list")
    program = next((item for item in programs if int(item["patch_code"]) == patch_code), None)
    if program is None:
        raise ValueError("Unknown patch_code; list the track's MIDI programs and select one of them")
    _require_muse_success(
        "setMidiTrackProgram", track_name, patch_code,
        failure_message=f"MusE rejected patch {patch_code} on {track_name!r}",
    )
    return {
        "track_name": track_name,
        "patch_code": patch_code,
        "program_name": str(program["name"]),
        "status": "updated",
    }


@mcp.tool(
    name="muse_list_available_track_effects",
    annotations={
        "title": "List Available Track Effects",
        "readOnlyHint": True,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_list_available_track_effects() -> list[dict[str, Any]]:
    """List audio effects available in MusE, including their plugin identifiers."""
    _require_profile("read_only")
    effects = list(_muse_call("getAvailableTrackEffects"))
    return [{**effect, "plugin_id": _effect_identifier(effect)} for effect in effects]


@mcp.tool(
    name="muse_list_track_effects",
    annotations={
        "title": "List Track Effects",
        "readOnlyHint": True,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_list_track_effects(track_name: TrackName) -> list[dict[str, Any]]:
    """List effect rack slots, plugin names, and enabled states for an audio track."""
    _require_profile("read_only")
    _require_track(track_name)
    effects = _muse_call("getTrackEffectDetails", track_name)
    if effects is None:
        raise ValueError(f"Track {track_name!r} does not support audio effects")
    return list(effects)


@mcp.tool(
    name="muse_list_track_effect_parameters",
    annotations={
        "title": "List Track Effect Parameters",
        "readOnlyHint": True,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_list_track_effect_parameters(
    track_name: TrackName,
    slot: Annotated[int, Field(ge=0, le=7)],
) -> list[dict[str, Any]]:
    """List parameter indices, names, current values, and accepted ranges for an effect."""
    _require_profile("read_only")
    _require_track(track_name)
    parameters = _muse_call("getTrackEffectParameters", track_name, slot)
    if parameters is None:
        raise ValueError(f"Track {track_name!r} has no effect in slot {slot}")
    return list(parameters)


@mcp.tool(
    name="muse_set_track_effect_parameter",
    annotations={
        "title": "Set Track Effect Parameter",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_set_track_effect_parameter(
    track_name: TrackName,
    slot: Annotated[int, Field(ge=0, le=7)],
    parameter_index: Annotated[int, Field(ge=0, le=1023)],
    value: Annotated[float, Field(ge=-1_000_000_000, le=1_000_000_000)],
) -> dict[str, Any]:
    """Set a listed effect parameter, rejecting values outside the plugin's reported range."""
    _require_profile("project_edit")
    _require_track(track_name)
    if not math.isfinite(value):
        raise ValueError("Effect parameter value must be finite")
    parameters = _muse_call("getTrackEffectParameters", track_name, slot)
    if parameters is None:
        raise ValueError(f"Track {track_name!r} has no effect in slot {slot}")
    parameter = next((item for item in parameters if int(item["index"]) == parameter_index), None)
    if parameter is None:
        raise ValueError(f"Effect slot {slot} has no parameter index {parameter_index}")
    minimum = float(parameter["minimum"])
    maximum = float(parameter["maximum"])
    if not math.isfinite(minimum) or not math.isfinite(maximum) or minimum > maximum:
        raise ValueError("MusE did not report a finite range for this effect parameter")
    if not minimum <= value <= maximum:
        raise ValueError(f"Parameter value must be between {minimum} and {maximum}")
    _require_muse_success(
        "setTrackEffectParameter", track_name, slot, parameter_index, value,
        failure_message="MusE rejected the effect parameter change",
    )
    return {
        "track_name": track_name,
        "slot": slot,
        "parameter_index": parameter_index,
        "parameter_name": str(parameter.get("name", "")),
        "value": value,
        "status": "updated",
    }


@mcp.tool(
    name="muse_add_track_effect",
    annotations={
        "title": "Add Track Effect",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": False,
    },
)
def muse_add_track_effect(
    track_name: TrackName,
    plugin_id: Annotated[str, Field(min_length=1, max_length=4096)],
    slot: Annotated[int, Field(ge=0, le=7)],
) -> dict[str, Any]:
    """Add an available effect to an empty track effect slot using its listed plugin_id."""
    _require_profile("project_edit")
    _require_track(track_name)
    rack = _muse_call("getTrackEffectDetails", track_name)
    if rack is None:
        raise ValueError(f"Track {track_name!r} does not support audio effects")
    target_slot = next((effect for effect in rack if int(effect["slot"]) == slot), None)
    if target_slot is None or target_slot.get("name"):
        raise ValueError(f"Track {track_name!r} has no empty effect slot {slot}")
    plugins = list(_muse_call("getAvailableTrackEffects"))
    plugin = next((effect for effect in plugins if _effect_identifier(effect) == plugin_id), None)
    if plugin is None:
        raise ValueError("Unknown effect plugin_id; list available track effects and use its plugin_id")
    _require_muse_success(
        "addTrackEffect", track_name, str(plugin.get("uri", "")), str(plugin.get("file", "")),
        str(plugin.get("label", "")), slot,
        failure_message="MusE rejected the effect; verify the plugin is installed and the slot remains empty",
    )
    return {"track_name": track_name, "slot": slot, "plugin_id": plugin_id, "status": "added"}


@mcp.tool(
    name="muse_set_track_effect_enabled",
    annotations={
        "title": "Enable or Bypass Track Effect",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": False,
    },
)
def muse_set_track_effect_enabled(
    track_name: TrackName,
    slot: Annotated[int, Field(ge=0, le=7)],
    enabled: bool,
) -> dict[str, Any]:
    """Enable or bypass an effect already installed in the selected rack slot."""
    _require_profile("project_edit")
    _require_track(track_name)
    rack = _muse_call("getTrackEffectDetails", track_name)
    if rack is None:
        raise ValueError(f"Track {track_name!r} does not support audio effects")
    effect = next((item for item in rack if int(item["slot"]) == slot), None)
    if effect is None or not effect.get("name"):
        raise ValueError(f"Track {track_name!r} has no effect in slot {slot}")
    _require_muse_success("toggleTrackEffect", track_name, slot, enabled)
    return {"track_name": track_name, "slot": slot, "enabled": enabled, "status": "updated"}


def _check_connection() -> dict[str, Any]:
    """Perform only read operations to verify the Pyro bridge connection."""
    return {
        "ok": True,
        "position_ticks": int(_muse_call("getCPos")),
        "tracks": list(_muse_call("getTrackNames")),
    }


if __name__ == "__main__":
    if sys.argv[1:] == ["--check-connection"]:
        try:
            print(json.dumps(_check_connection()))
        except Exception as exc:
            print(json.dumps({"ok": False, "error": str(exc)}))
            sys.exit(1)
    else:
        mcp.run()