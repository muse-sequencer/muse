

## is libpython.so linked into muse , is it used ? 


Yes, it's genuinely used — not dead weight. Here's the picture:

**Why `libpython3.12.so` gets linked:** the `remote/` module (`pyapi.cpp`/`pyapi.h`) embeds a real Python interpreter into MusE, guarded by the `PYTHON_SUPPORT` CMake option. It's only built and linked at all when that option is enabled — it's not unconditional.

**Is it actually used?** Yes:
- `pyapi.cpp` calls `Py_Initialize()` and `PyRun_File()` directly — this is a genuine embedded interpreter, not vestigial scaffolding.
- It's launched from `-y` command-line flag handling (`main.cpp`), which enables "Python control support," along with `--pyro-ns-host`/`--pyro-ns-port` options.
- `app.cpp` calls `startPythonBridge()`/`stopPythonBridge()` during startup/shutdown when `MusEGlobal::usePythonBridge` is set.

**What it's for:** it's a remote-control bridge built on **Pyro** (Python Remote Objects — an old Python RPC library), exposing a `PyMethodDef` table (`pyapi.cpp:1048`) of MusE functions callable from external Python scripts over that RPC connection. So the intent is scripting/remote automation of MusE from Python, not something used internally by the C++ core for its own logic.

**Practically:** if you don't need Python scripting of MusE, you can build with `PYTHON_SUPPORT` off (however your build config wires that CMake option) and `libpython3.12.so` won't be linked at all — nothing else in the codebase depends on it once that flag is off. Worth noting the code comment right above the startup call literally says `// Uncomment in order to enable MusE Python bridge:`, suggesting even the upstream MusE maintainers treat this as an opt-in, somewhat experimental feature rather than a core dependency.
