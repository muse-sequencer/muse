"""Dependency-free tests for methods exposed by MusE's Pyro bridge."""

import importlib.util
import sys
import types
import unittest
from pathlib import Path
from unittest.mock import Mock, patch


def load_pybridge_module(muse_module):
    def identity_decorator(function):
        return function

    pyro_package = types.ModuleType("Pyro5")
    pyro_api = types.ModuleType("Pyro5.api")
    pyro_api.expose = identity_decorator
    pyro_api.oneway = identity_decorator
    pyro_errors = types.ModuleType("Pyro5.errors")
    pyro_errors.NamingError = type("NamingError", (Exception,), {})
    pyro_nameserver = types.ModuleType("Pyro5.nameserver")
    pyro_nameserver.start_ns_loop = Mock()
    pyro_package.api = pyro_api
    pyro_package.errors = pyro_errors

    musepyroconfig = types.ModuleType("musepyroconfig")
    musepyroconfig.normalize_bind_host = lambda host: host

    modules = {
        "Pyro5": pyro_package,
        "Pyro5.api": pyro_api,
        "Pyro5.errors": pyro_errors,
        "Pyro5.nameserver": pyro_nameserver,
        "muse": muse_module,
        "musepyroconfig": musepyroconfig,
    }
    with patch.dict(sys.modules, modules):
        source = Path(__file__).parent / "src/share/pybridge/museplauncher.py"
        spec = importlib.util.spec_from_file_location("museplauncher_under_test", source)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
    return module


class PyroProjectMethodTests(unittest.TestCase):
    def test_project_operations_are_exposed_and_forwarded(self):
        muse_module = types.ModuleType("muse")
        operations = {
            "saveProjectFile": ("/tmp/song.med", True),
            "closeProjectFile": ("/tmp/song.med", True),
            "createProjectFile": ("/tmp/new.med", True, "/tmp/song.med", False),
            "openProjectFile": ("/tmp/next.med", False, "/tmp/song.med", True),
            "getMidiOutputs": (),
            "getMidiTrackOutput": ("Piano",),
            "setMidiTrackOutput": ("Piano", 1, 0),
            "getMidiTrackPrograms": ("Piano",),
            "setMidiTrackProgram": ("Piano", 17),
            "getAvailableInstruments": (),
            "addSynthTrack": (6, "", "urn:organ", "organ", "Organ"),
            "getAvailableTrackEffects": (),
            "getTrackEffectDetails": ("Audio",),
            "getTrackEffectParameters": ("Audio", 0),
            "setTrackEffectParameter": ("Audio", 0, 1, 0.5),
            "addTrackEffect": ("Audio", "urn:reverb", "", "reverb", 0),
        }
        delegates = {}
        for name in operations:
            delegate = Mock(return_value=name)
            setattr(muse_module, name, delegate)
            delegates[name] = delegate

        bridge = load_pybridge_module(muse_module)
        remote = bridge.MusE(Mock())

        for name, args in operations.items():
            with self.subTest(operation=name):
                self.assertEqual(getattr(remote, name)(*args), name)
                delegates[name].assert_called_once_with(*args)


if __name__ == "__main__":
    unittest.main()