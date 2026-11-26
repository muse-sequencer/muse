"""Dependency-free unit tests for the MusE MCP adapter."""

import importlib.util
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest.mock import Mock, patch


class FakeFastMCP:
    def __init__(self, name):
        self.name = name
        self.tools = {}

    def tool(self, *, name, annotations):
        def register(function):
            self.tools[name] = function
            return function

        return register


class FakeModel:
    def __init__(self, **values):
        self.__dict__.update(values)


def load_server_module():
    mcp_package = types.ModuleType("mcp")
    mcp_server = types.ModuleType("mcp.server")
    mcp_fastmcp = types.ModuleType("mcp.server.fastmcp")
    mcp_fastmcp.FastMCP = FakeFastMCP

    pyro_package = types.ModuleType("Pyro5")
    pyro_api = types.ModuleType("Pyro5.api")
    pyro_api.locate_ns = None
    pyro_api.Proxy = None
    pyro_errors = types.ModuleType("Pyro5.errors")

    class FakePyroError(Exception):
        pass

    pyro_errors.PyroError = FakePyroError
    pyro_package.api = pyro_api
    pyro_package.errors = pyro_errors

    pydantic = types.ModuleType("pydantic")
    pydantic.BaseModel = FakeModel
    pydantic.Field = lambda *args, **kwargs: object()

    modules = {
        "mcp": mcp_package,
        "mcp.server": mcp_server,
        "mcp.server.fastmcp": mcp_fastmcp,
        "Pyro5": pyro_package,
        "Pyro5.api": pyro_api,
        "Pyro5.errors": pyro_errors,
        "pydantic": pydantic,
    }
    bridge_path = Path(__file__).parent / "src" / "share" / "pybridge"
    server_path = bridge_path / "musemcp.py"
    spec = importlib.util.spec_from_file_location("musemcp_test_instance", server_path)
    module = importlib.util.module_from_spec(spec)
    with patch.dict(sys.modules, modules):
        with patch.object(sys, "path", [str(bridge_path), *sys.path]):
            spec.loader.exec_module(module)
    return module


class MuseMcpTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = load_server_module()

    def test_project_creation_tool_is_registered_with_mcp(self):
        self.assertIn("muse_create_project", self.server.mcp.tools)

    def test_midi_track_output_read_tool_is_registered_with_mcp(self):
        self.assertIn("muse_get_midi_track_output", self.server.mcp.tools)

    def test_proxy_call_looks_up_and_releases_bridge_connections(self):
        nameserver = Mock()
        nameserver.lookup.return_value = "PYRO:object@127.0.0.1:50000"
        proxy = Mock()
        proxy.getCPos.return_value = 240

        with patch.dict(self.server.os.environ, {
            "MUSE_PYRO_NS_HOST": "127.0.0.1",
            "MUSE_PYRO_NS_PORT": "",
            "MUSE_PYRO_TIMEOUT": "5",
        }):
            with patch.object(self.server.pyro, "locate_ns", return_value=nameserver) as locate_ns:
                with patch.object(self.server.pyro, "Proxy", return_value=proxy):
                    self.assertEqual(self.server._muse_call("getCPos"), 240)

        locate_ns.assert_called_once_with(host="127.0.0.1", port=None)
        nameserver.lookup.assert_called_once_with("muse")
        proxy._pyroRelease.assert_called_once_with()
        nameserver._pyroRelease.assert_called_once_with()

    def test_project_file_calls_wait_without_rpc_timeout(self):
        nameserver = Mock()
        nameserver.lookup.return_value = "PYRO:object@127.0.0.1:50000"
        proxy = Mock()
        proxy.saveProjectFile.return_value = True

        with patch.dict(self.server.os.environ, {"MUSE_PYRO_TIMEOUT": "5"}):
            with patch.object(self.server.pyro, "locate_ns", return_value=nameserver):
                with patch.object(self.server.pyro, "Proxy", return_value=proxy):
                    self.assertTrue(self.server._muse_call("saveProjectFile", "/tmp/Bach.med", False))

        self.assertIsNone(proxy._pyroTimeout)

    def test_project_creation_waits_without_rpc_timeout(self):
        nameserver = Mock()
        nameserver.lookup.return_value = "PYRO:object@127.0.0.1:50000"
        proxy = Mock()
        proxy.createProjectFile.return_value = True

        with patch.dict(self.server.os.environ, {"MUSE_PYRO_TIMEOUT": "5"}):
            with patch.object(self.server.pyro, "locate_ns", return_value=nameserver):
                with patch.object(self.server.pyro, "Proxy", return_value=proxy):
                    self.assertTrue(
                        self.server._muse_call("createProjectFile", "/tmp/NewProject.med", True, "", False)
                    )

        self.assertIsNone(proxy._pyroTimeout)

    def test_non_project_calls_keep_the_configured_rpc_timeout(self):
        nameserver = Mock()
        nameserver.lookup.return_value = "PYRO:object@127.0.0.1:50000"
        proxy = Mock()
        proxy.getCPos.return_value = 120

        with patch.dict(self.server.os.environ, {"MUSE_PYRO_TIMEOUT": "7"}):
            with patch.object(self.server.pyro, "locate_ns", return_value=nameserver):
                with patch.object(self.server.pyro, "Proxy", return_value=proxy):
                    self.assertEqual(self.server._muse_call("getCPos"), 120)

        self.assertEqual(proxy._pyroTimeout, 7.0)

    def test_connection_error_explains_how_to_enable_bridge(self):
        with patch.object(self.server.pyro, "locate_ns", side_effect=self.server.PyroError("offline")):
            with self.assertRaisesRegex(RuntimeError, "muse -y"):
                self.server._muse_call("getCPos")

    def test_nameserver_host_defaults_to_loopback_and_rejects_public_hosts(self):
        with patch.dict(self.server.os.environ, {}, clear=True):
            self.assertEqual(self.server._nameserver_options(), ("127.0.0.1", None, 5.0))
        with patch.dict(self.server.os.environ, {"MUSE_PYRO_NS_HOST": "8.8.8.8"}, clear=True):
            with self.assertRaisesRegex(ValueError, "private IPv4"):
                self.server._nameserver_options()

    def test_nameserver_host_accepts_private_network_address(self):
        for address in ("10.0.0.5", "172.16.2.3", "192.168.1.10"):
            with self.subTest(address=address):
                with patch.dict(self.server.os.environ, {"MUSE_PYRO_NS_HOST": address}, clear=True):
                    self.assertEqual(self.server._nameserver_options()[0], address)

    def test_read_only_connection_check_only_reads_position_and_tracks(self):
        with patch.object(self.server, "_muse_call", side_effect=[120, ["Piano"]]) as call:
            self.assertEqual(self.server._check_connection(), {
                "ok": True,
                "position_ticks": 120,
                "tracks": ["Piano"],
            })
        call.assert_has_calls([
            unittest.mock.call("getCPos"),
            unittest.mock.call("getTrackNames"),
        ])

    def test_song_state_returns_tick_and_tempo_metadata(self):
        values = [240, 3840, 480, 500000, 0, 1920, False, ["Piano"]]
        with patch.object(self.server, "_muse_call", side_effect=values):
            state = self.server.muse_get_song_state()

        self.assertEqual(state["position_ticks"], 240)
        self.assertEqual(state["tempo_us_per_quarter_note"], 500000)
        self.assertEqual(state["tracks"], ["Piano"])

    def test_set_position_rejects_values_outside_bridge_integer_range(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "transport"}):
            with patch.object(self.server, "_muse_call") as call:
                with self.assertRaisesRegex(ValueError, "between zero"):
                    self.server.muse_set_position(self.server.MAX_TICK + 1)
        call.assert_not_called()

    def test_create_midi_part_translates_note_fields_to_bridge_event(self):
        notes = [self.server.MidiNote(pitch=60, velocity=100, tick=12, length_ticks=48)]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Piano"], [], "part-uuid"]) as call:
                result = self.server.muse_create_midi_part("Piano", 960, 1920, notes)

        self.assertEqual(result["note_count"], 1)
        self.assertEqual(result["part_id"], "part-uuid")
        call.assert_any_call("getTrackNames")
        call.assert_any_call("createPart", "Piano", 960, 1920, {
            "tick": 0,
            "events": [{"data": [60, 100, 0], "tick": 12, "type": "note", "len": 48}],
        })

    def test_list_midi_parts_returns_bridge_part_metadata(self):
        parts = [{"id": "part-uuid", "tick": 960, "len": 1920, "events": []}]
        with patch.object(self.server, "_muse_call", side_effect=[["Piano"], parts]) as call:
            result = self.server.muse_list_midi_parts("Piano")

        self.assertEqual(result, parts)
        call.assert_has_calls([
            unittest.mock.call("getTrackNames"),
            unittest.mock.call("getParts", "Piano"),
        ])

    def test_update_midi_part_moves_resizes_and_preserves_notes(self):
        existing = {
            "id": "part-uuid",
            "tick": 960,
            "len": 1920,
            "events": [{"data": [60, 100, 0], "tick": 12, "type": "note", "len": 48}],
        }
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Piano"], [existing], True]) as call:
                result = self.server.muse_update_midi_part("Piano", "part-uuid", 1440, 960)

        self.assertEqual(result["start_tick"], 1440)
        call.assert_called_with("modifyPart", {
            "id": "part-uuid",
            "tick": 1440,
            "len": 960,
            "events": existing["events"],
        })

    def test_import_midi_file_scales_notes_to_song_division(self):
        class FakeMidiFile:
            ticks_per_beat = 480
            tracks = [[
                types.SimpleNamespace(type="note_on", time=120, note=64, velocity=90, channel=3),
                types.SimpleNamespace(type="control_change", time=120, control=1, value=64, channel=2),
                types.SimpleNamespace(type="note_off", time=120, note=64, velocity=0, channel=3),
            ]]

        with tempfile.TemporaryDirectory() as temp_dir:
            midi_path = Path(temp_dir) / "song.mid"
            midi_path.write_bytes(b"MThd")
            with patch.dict(sys.modules, {"mido": types.SimpleNamespace(MidiFile=lambda path: FakeMidiFile())}):
                with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
                    with patch.object(self.server, "_muse_call", side_effect=[["Piano"], [], 384, "imported-part"]) as call:
                        result = self.server.muse_import_midi_file("Piano", str(midi_path), 100)

        self.assertEqual(result["part_id"], "imported-part")
        self.assertEqual(result["note_count"], 1)
        self.assertEqual(result["controller_event_count"], 1)
        call.assert_called_with("createPart", "Piano", 100, 288, {
            "tick": 0,
            "events": [
                {"data": [64, 90, 3], "tick": 96, "type": "note", "len": 192},
                {"data": [1, 64, 2], "tick": 192, "type": "ctrl"},
            ],
        })

    def test_import_midi_file_rejects_out_of_range_controller_fields(self):
        invalid_controllers = (
            {"control": -1, "value": 64, "channel": 0},
            {"control": 128, "value": 64, "channel": 0},
            {"control": 1, "value": -1, "channel": 0},
            {"control": 1, "value": 128, "channel": 0},
            {"control": 1, "value": 64, "channel": -1},
            {"control": 1, "value": 64, "channel": 16},
        )
        for controller in invalid_controllers:
            with self.subTest(controller=controller):
                midi_file = types.SimpleNamespace(
                    ticks_per_beat=480,
                    tracks=[[types.SimpleNamespace(type="control_change", time=0, **controller)]],
                )
                with patch.dict(sys.modules, {"mido": types.SimpleNamespace(MidiFile=lambda path: midi_file)}):
                    with self.assertRaisesRegex(ValueError, "controller.*out of range"):
                        self.server._read_midi_file(Path("unused.mid"), None, 384)

    def test_import_midi_file_rejects_empty_file_before_bridge_call(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            missing_path = str(Path(temp_dir) / "missing.mid")
            with self.assertRaisesRegex(ValueError, "MIDI file"):
                self.server._validate_midi_file_path(missing_path)

    def test_rejects_note_outside_part_before_bridge_call(self):
        notes = [self.server.MidiNote(pitch=60, velocity=100, tick=100, length_ticks=20)]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call") as call:
                with self.assertRaisesRegex(ValueError, "within the part length"):
                    self.server.muse_create_midi_part("Piano", 0, 100, notes)
        call.assert_not_called()

    def test_add_midi_track_rejects_duplicate_name(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", return_value=["Piano"]):
                with self.assertRaisesRegex(ValueError, "already exists"):
                    self.server.muse_add_midi_track("Piano")

    def test_add_midi_track_waits_for_named_bridge_result(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[[], True, ["Strings"]]) as call:
                result = self.server.muse_add_midi_track("Strings")

        self.assertEqual(result, {"track_name": "Strings", "track_type": "midi"})
        call.assert_has_calls([
            unittest.mock.call("getTrackNames"),
            unittest.mock.call("addMidiTrack", "Strings"),
            unittest.mock.call("getTrackNames"),
        ])

    def test_list_available_lv2_synth_adds_its_uri_as_plugin_id(self):
        instruments = [{
            "name": "Yoshimi LV2", "type": 32, "file": "",
            "uri": "http://yoshimi.sourceforge.net/lv2_plugin", "label": "Yoshimi",
        }]
        with patch.object(self.server, "_muse_call", return_value=instruments):
            result = self.server.muse_list_available_instruments()

        self.assertEqual(result[0]["plugin_id"], "http://yoshimi.sourceforge.net/lv2_plugin")

    def test_add_lv2_synth_track_uses_listed_plugin_and_waits_for_creation(self):
        instrument = {
            "name": "Yoshimi LV2", "type": 32, "file": "",
            "uri": "http://yoshimi.sourceforge.net/lv2_plugin", "label": "Yoshimi",
        }
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[[], [instrument], 2, ["Yoshimi"]]) as call:
                result = self.server.muse_add_instrument_track(
                    "http://yoshimi.sourceforge.net/lv2_plugin", "Yoshimi"
                )

        self.assertEqual(result["status"], "created")
        self.assertEqual(result["midi_output_port"], 2)
        call.assert_has_calls([
            unittest.mock.call("getTrackNames"),
            unittest.mock.call("getAvailableInstruments"),
            unittest.mock.call(
                "addSynthTrack", 32, "", "http://yoshimi.sourceforge.net/lv2_plugin", "Yoshimi", "Yoshimi"
            ),
            unittest.mock.call("getTrackNames"),
        ])

    def test_add_instrument_track_rejects_unknown_plugin(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[[], []]) as call:
                with self.assertRaisesRegex(ValueError, "Unknown instrument plugin_id"):
                    self.server.muse_add_instrument_track("missing", "Organ")
        self.assertNotIn(unittest.mock.call("addSynthTrack"), call.call_args_list)

    def test_add_instrument_track_rejects_unconfirmed_port_allocation(self):
        instrument = {"type": 32, "name": "Yoshimi", "uri": "urn:yoshimi"}
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[[], [instrument], True]):
                with self.assertRaisesRegex(RuntimeError, "did not confirm instrument creation"):
                    self.server.muse_add_instrument_track("urn:yoshimi", "Yoshimi")

    def test_set_midi_track_output_rejects_unavailable_output(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Piano"], [], []]) as call:
                with self.assertRaisesRegex(ValueError, "output port"):
                    self.server.muse_set_midi_track_output("Piano", 3, 0)
        call.assert_has_calls([
            unittest.mock.call("getTrackNames"),
            unittest.mock.call("getParts", "Piano"),
            unittest.mock.call("getMidiOutputs"),
        ])

    def test_add_track_effect_requires_an_empty_slot_and_known_uri(self):
        effects = [{"slot": 0, "name": "Compressor", "enabled": True, "uri": "urn:compressor"}]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Audio"], effects, [{"uri": "urn:reverb"}]]) as call:
                with self.assertRaisesRegex(ValueError, "empty effect slot"):
                    self.server.muse_add_track_effect("Audio", "urn:reverb", 0)
        self.assertNotIn(unittest.mock.call("addTrackEffect", "Audio", "urn:reverb", 0), call.call_args_list)

    def test_set_midi_track_output_routes_to_a_listed_synth_port(self):
        outputs = [{"port": 2, "name": "Synth"}]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Piano"], [], outputs, True]) as call:
                result = self.server.muse_set_midi_track_output("Piano", 2, 4)

        self.assertEqual(result["output_name"], "Synth")
        call.assert_called_with("setMidiTrackOutput", "Piano", 2, 4)

    def test_get_midi_track_output_reports_assigned_port_and_channel(self):
        routing = {"port": 2, "channel": 4}
        outputs = [{"port": 2, "name": "Yoshimi LV2"}]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "read_only"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Piano"], [], routing, outputs]) as call:
                result = self.server.muse_get_midi_track_output("Piano")

        self.assertEqual(result, {
            "track_name": "Piano",
            "port_index": 2,
            "output_name": "Yoshimi LV2",
            "channel": 4,
            "output_assigned": True,
        })
        call.assert_has_calls([
            unittest.mock.call("getTrackNames"),
            unittest.mock.call("getParts", "Piano"),
            unittest.mock.call("getMidiTrackOutput", "Piano"),
            unittest.mock.call("getMidiOutputs"),
        ])

    def test_get_midi_track_output_reports_unassigned_port(self):
        with patch.object(self.server, "_muse_call", side_effect=[["Piano"], [], {"port": 1, "channel": 0}, []]):
            result = self.server.muse_get_midi_track_output("Piano")

        self.assertEqual(result["port_index"], 1)
        self.assertIsNone(result["output_name"])
        self.assertFalse(result["output_assigned"])

    def test_set_midi_track_output_reports_bridge_failure_value(self):
        outputs = [{"port": 2, "name": "Synth"}]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Piano"], [], outputs, False]):
                with self.assertRaisesRegex(RuntimeError, "MusE could not route.*bridge returned False"):
                    self.server.muse_set_midi_track_output("Piano", 2, 4)

    def test_list_midi_track_programs_uses_assigned_instrument(self):
        programs = [{"patch_code": 19, "program": 19, "bank_lsb": 255, "bank_msb": 255, "name": "Church Organ"}]
        with patch.object(self.server, "_muse_call", side_effect=[["Organ"], [], programs]):
            result = self.server.muse_list_midi_track_programs("Organ")

        self.assertEqual(result[0]["name"], "Church Organ")

    def test_set_midi_track_program_uses_listed_patch_code(self):
        programs = [{"patch_code": 19, "program": 19, "bank_lsb": 255, "bank_msb": 255, "name": "Church Organ"}]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Organ"], [], programs, True]) as call:
                result = self.server.muse_set_midi_track_program("Organ", 19)

        self.assertEqual(result["program_name"], "Church Organ")
        call.assert_called_with("setMidiTrackProgram", "Organ", 19)

    def test_set_midi_track_program_rejects_unlisted_patch(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Organ"], [], []]) as call:
                with self.assertRaisesRegex(ValueError, "Unknown patch_code"):
                    self.server.muse_set_midi_track_program("Organ", 19)
        self.assertNotIn(unittest.mock.call("setMidiTrackProgram"), call.call_args_list)

    def test_add_track_effect_uses_listed_plugin_identity(self):
        effects = [{"slot": i, "name": "", "enabled": False, "uri": ""} for i in range(8)]
        plugins = [{"name": "Reverb", "uri": "urn:reverb", "file": "", "label": ""}]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Audio"], effects, plugins, True]) as call:
                result = self.server.muse_add_track_effect("Audio", "urn:reverb", 3)

        self.assertEqual(result["status"], "added")
        call.assert_called_with("addTrackEffect", "Audio", "urn:reverb", "", "", 3)

    def test_list_track_effect_parameters_returns_native_ranges(self):
        parameters = [{"index": 0, "name": "Room size", "value": 0.5, "minimum": 0.0, "maximum": 1.0}]
        with patch.object(self.server, "_muse_call", side_effect=[["Audio"], parameters]):
            result = self.server.muse_list_track_effect_parameters("Audio", 2)

        self.assertEqual(result, parameters)

    def test_set_track_effect_parameter_uses_reported_index_and_range(self):
        parameters = [{"index": 1, "name": "Decay", "value": 0.5, "minimum": 0.1, "maximum": 10.0}]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Audio"], parameters, True]) as call:
                result = self.server.muse_set_track_effect_parameter("Audio", 2, 1, 4.0)

        self.assertEqual(result["parameter_name"], "Decay")
        call.assert_called_with("setTrackEffectParameter", "Audio", 2, 1, 4.0)

    def test_set_track_effect_parameter_rejects_out_of_range_value(self):
        parameters = [{"index": 0, "name": "Wet", "value": 0.5, "minimum": 0.0, "maximum": 1.0}]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Audio"], parameters]) as call:
                with self.assertRaisesRegex(ValueError, "between 0.0 and 1.0"):
                    self.server.muse_set_track_effect_parameter("Audio", 2, 0, 1.5)
        self.assertNotIn(unittest.mock.call("setTrackEffectParameter"), call.call_args_list)

    def test_save_project_adds_med_extension_and_calls_bridge(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            path = Path(temporary_directory) / "Bach"
            with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
                with patch.object(self.server, "_muse_call", return_value=True) as call:
                    result = self.server.muse_save_project(str(path), overwrite=False)

        expected_path = str(path.with_suffix(".med").resolve())
        self.assertEqual(result["file_path"], expected_path)
        call.assert_called_once_with("saveProjectFile", expected_path, False)

    def test_save_project_passes_explicit_overwrite_permission(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            path = Path(temporary_directory) / "Bach.med"
            with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
                with patch.object(self.server, "_muse_call", return_value=True) as call:
                    result = self.server.muse_save_project(str(path), overwrite=True)

        self.assertEqual(result["status"], "saved")
        call.assert_called_once_with("saveProjectFile", str(path.resolve()), True)

    def test_save_project_rejects_non_med_extension_before_bridge_call(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call") as call:
                with self.assertRaisesRegex(ValueError, "must end in .med"):
                    self.server.muse_save_project("Bach.txt")
        call.assert_not_called()

    def test_project_save_path_can_be_outside_the_active_project_directory(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            requested_path = Path(temporary_directory) / "external" / "Bach.med"
            self.assertEqual(
                self.server._validate_project_save_path(str(requested_path)),
                requested_path.resolve(),
            )

    def test_close_project_requests_save_before_close(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", return_value="/tmp/Bach.med") as call:
                result = self.server.muse_close_project()

        self.assertEqual(result, {"status": "closed", "saved_to": "/tmp/Bach.med"})
        call.assert_called_once_with("closeProjectFile", "", False)

    def test_close_project_rejects_bridge_close_without_saved_path(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", return_value=False):
                with self.assertRaisesRegex(RuntimeError, "could not save and close"):
                    self.server.muse_close_project()

    def test_create_project_adds_med_extension_and_calls_bridge(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            path = Path(temporary_directory) / "NewProject"
            with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
                with patch.object(self.server, "_muse_call", return_value=True) as call:
                    result = self.server.muse_create_project(str(path))

        expected_path = str(path.with_suffix(".med").resolve())
        self.assertEqual(result, {"file_path": expected_path, "status": "created"})
        call.assert_called_once_with("createProjectFile", expected_path, True, "", False)

    def test_create_project_passes_current_save_and_overwrite_policy(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            path = Path(temporary_directory) / "NewProject.med"
            save_path = Path(temporary_directory) / "CurrentProject"
            with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
                with patch.object(self.server, "_muse_call", return_value=True) as call:
                    self.server.muse_create_project(
                        str(path), save_current=True, save_current_path=str(save_path), overwrite=True
                    )

        call.assert_called_once_with(
            "createProjectFile", str(path.resolve()), True, str(save_path.with_suffix(".med").resolve()), True
        )

    def test_create_project_rejects_existing_file_without_overwrite(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            path = Path(temporary_directory) / "Existing.med"
            path.write_text("existing", encoding="utf-8")
            with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
                with patch.object(self.server, "_muse_call") as call:
                    with self.assertRaisesRegex(ValueError, "already exists"):
                        self.server.muse_create_project(str(path))
        call.assert_not_called()

    def test_open_project_requires_readable_med_file_and_passes_save_policy(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            path = Path(temporary_directory) / "Bach.med"
            path.write_text("test project", encoding="utf-8")
            with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
                with patch.object(self.server, "_muse_call", return_value=True) as call:
                    result = self.server.muse_open_project(str(path), save_current=True)

        expected_path = str(path.resolve())
        self.assertEqual(result["file_path"], expected_path)
        call.assert_called_once_with("openProjectFile", expected_path, True, "", False)

    def test_open_project_rejects_missing_file_before_bridge_call(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            path = Path(temporary_directory) / "missing.med"
            with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
                with patch.object(self.server, "_muse_call") as call:
                    with self.assertRaisesRegex(ValueError, "readable MusE .med project"):
                        self.server.muse_open_project(str(path))
        call.assert_not_called()

    def test_project_management_tools_require_project_edit_profile(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "transport"}):
            with patch.object(self.server, "_muse_call") as call:
                with self.assertRaisesRegex(PermissionError, "project_edit"):
                    self.server.muse_close_project()
                with self.assertRaisesRegex(PermissionError, "project_edit"):
                    self.server.muse_create_project("/tmp/NewProject.med")
        call.assert_not_called()

    def test_new_track_mutations_require_project_edit_profile(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "transport"}):
            with patch.object(self.server, "_muse_call") as call:
                with self.assertRaisesRegex(PermissionError, "project_edit"):
                    self.server.muse_set_midi_track_output("Piano", 0)
        call.assert_not_called()

    def test_transport_profile_blocks_project_edits(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "transport"}):
            with patch.object(self.server, "_muse_call") as call:
                with self.assertRaisesRegex(PermissionError, "project_edit"):
                    self.server.muse_add_midi_track("Strings")
        call.assert_not_called()

    def test_read_only_profile_blocks_transport_before_bridge_call(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "read_only"}):
            with patch.object(self.server, "_muse_call") as call:
                with self.assertRaisesRegex(PermissionError, "transport"):
                    self.server.muse_set_position(240)
        call.assert_not_called()

    def test_unknown_profile_fails_closed(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "admin"}):
            with patch.object(self.server, "_muse_call") as call:
                with self.assertRaisesRegex(PermissionError, "Unknown MUSE_MCP_PROFILE"):
                    self.server.muse_get_song_state()
        call.assert_not_called()

    def test_playback_fails_when_bridge_reports_failure_or_no_result(self):
        for result in (False, None):
            with self.subTest(result=result):
                with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "transport"}):
                    with patch.object(self.server, "_muse_call", return_value=result) as call:
                        with self.assertRaises(RuntimeError):
                            self.server.muse_start_playback()
                call.assert_called_once_with("startPlay")

    def test_track_mute_fails_when_bridge_reports_failure(self):
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Piano"], False]) as call:
                with self.assertRaises(RuntimeError):
                    self.server.muse_set_track_mute("Piano", True)
        call.assert_called_with("setMute", "Piano", True)

    def test_track_effect_enable_fails_when_bridge_reports_failure(self):
        rack = [{"slot": 0, "name": "Hall Reverb"}]
        with patch.dict(self.server.os.environ, {"MUSE_MCP_PROFILE": "project_edit"}):
            with patch.object(self.server, "_muse_call", side_effect=[["Piano"], rack, False]) as call:
                with self.assertRaises(RuntimeError):
                    self.server.muse_set_track_effect_enabled("Piano", 0, True)
        call.assert_called_with("toggleTrackEffect", "Piano", 0, True)


if __name__ == "__main__":
    unittest.main()