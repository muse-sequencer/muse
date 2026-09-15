

CLAP GUI support is a cooperative protocol between host and plugin. Here's how it works:

**The host's responsibilities:**

1. **Announce GUI support** — the host's `get_extension()` must return a `clap_host_gui_t` vtable (already done in our code via `s_hostGuiExtension`). This tells the plugin "I can receive GUI requests."

2. **Create a native window or provide a parent** — the host calls `plugin->gui->create(api, is_floating)`, then either:
   - **Embedded**: calls `gui->set_parent(&clap_window)` with a native window handle (X11 `Window`, Win32 `HWND`, Cocoa `NSView*`)
   - **Floating**: calls `gui->set_transient(&clap_window)` so the plugin can parent its own floating window to yours

3. **Show/hide/destroy** — `gui->show()`, `gui->hide()`, `gui->destroy()` are host-driven calls.

**The plugin's responsibilities:**

The plugin creates and manages its own GUI window contents entirely. It just needs a parent handle from the host. The plugin calls back into the host (via `clap_host_gui_t`) when:
- It wants to resize: `host->gui->request_resize(width, height)`
- It was closed by the user: `host->gui->closed(was_destroyed)`
- It wants to show/hide itself: `host->gui->request_show()` / `request_hide()`

---

**What's missing in our current code for a working embedded GUI:**

The `setParentWindow(WId parentWindow)` piece is not yet there. You need something like:

```cpp
void ClapSynthIF::setParentWindow(WId parentWindow)
{
  if(!_extGui || !_plugin)
    return;

  if(_isGuiCreated)
    closeNativeGui();

  const char* api = CLAP_WINDOW_API_X11; // Linux

  bool floating = false;
  if(!_extGui->is_api_supported(_plugin, api, false)) {
    if(!_extGui->is_api_supported(_plugin, api, true)) {
      fprintf(stderr, "ClapSynthIF: plugin has no supported GUI api\n");
      return;
    }
    floating = true;
  }

  if(!_extGui->create(_plugin, api, floating)) {
    fprintf(stderr, "ClapSynthIF: gui->create() failed\n");
    return;
  }
  _isGuiCreated = true;
  _isGuiFloating = floating;

  clap_window_t w;
  w.api  = api;
  w.x11  = (unsigned long)parentWindow; // X11 Window ID

  if(!floating) {
    uint32_t width = 0, height = 0;
    if(_extGui->get_size(_plugin, &width, &height)) {
      // resize the MusE plugin widget to width x height here
    }
    _extGui->set_parent(_plugin, &w);
  } else {
    _extGui->set_transient(_plugin, &w);
  }

  _extGui->show(_plugin);
  _isGuiVisible = true;
}
```

And then `clapHostGuiRequestResize` needs to actually resize the Qt widget that hosts the plugin view — that's the one real integration point where MusE's UI layer needs to respond.

---

**Short answer:** the plugin draws everything itself. The host just needs to hand it a native window handle and respond to resize/close callbacks. The most significant missing piece in our file is `setParentWindow()` connecting the MusE plugin widget's `WId` (X11 `Window` on Linux) to `gui->set_parent()`.

