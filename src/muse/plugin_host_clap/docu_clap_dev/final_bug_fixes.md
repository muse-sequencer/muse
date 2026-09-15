

# CLAP GUI Integration: Bug Fix Report

This document details the series of bugs encountered while implementing X11/Qt GUI embedding for CLAP plugins inside the MusE sequencer, and the steps taken to resolve them.


## Modified Files

The following files were modified during this debugging session:

* `clap_host_gui.cpp` (Main GUI logic, window reparenting, event loop management)

* `clap_host.cpp` (Core host implementation, lifecycle management)


## Bug 1: Plugin Window is Empty / Does Not Paint

**Symptom:** The host successfully creates the embedding container and reparents the X11 window, but the plugin GUI remains completely blank/transparent.
**Root Cause(s):** 1. **Late Extension Exposure:** Some plugins (e.g., u-he) do not expose `timer-support` and `posix-fd-support` during the initial plugin `init()`. They only become available *during* or *after* `gui->create()`. Because the host queried them too early and cached `nullptr`, the plugin's event loop was never started.
2. **Qt Overdraw:** Qt aggressively repaints the background of `QWidget` containers, which can overwrite the embedded plugin's OpenGL/Vulkan rendering.

**Solution:**
Dynamically re-query the extensions if they are `nullptr` when the plugin attempts to register timers or file descriptors. Tell Qt to stop painting the system background for the container widget.

**Code Snippet (`clap_host_gui.cpp`):**

```
// 1. Dynamic extension query
bool ClapSynthIF::hostTimerRegister(uint32_t period_ms, clap_id* timer_id) {
  if(!_extTimer) {
    _extTimer = static_cast<const clap_plugin_timer_support_t*>(
                  _plugin->get_extension(_plugin, CLAP_EXT_TIMER_SUPPORT));
  }
  // ... 
}

// 2. Prevent Qt overdraw
_editorWindow->setAttribute(Qt::WA_OpaquePaintEvent, true);
_editorWindow->setAttribute(Qt::WA_NoSystemBackground, true);

```


## Bug 2: Pure Black Window on Re-Show

**Symptom:** After hiding the plugin window and showing it again, the window appears purely black.
**Root Cause:** When an embedded X11 window is unmapped (hidden), hardware-accelerated plugins often lose their graphics context (Swapchain/Surface) or fail to respond correctly to subsequent `Expose` events.
**Solution:** Instead of merely hiding the window, completely tear down the GUI (`gui->destroy()`) and recreate it from scratch upon reopening. This is the industry standard for Linux DAWs to ensure a clean graphics context.

**Code Snippet (`clap_host_gui.cpp`):**

```
void ClapSynthIF::showNativeGui(bool v) {
  // ...
  } else {
    if(_isGuiVisible) {
      // Destroying the GUI completely on hide is the most reliable approach for Linux.
      destroyGui(); 
    }
  }
}

```


## Bug 3: Black Screen on the *Second* Re-Show

**Symptom:** After implementing the `destroyGui()` fix, the window opens fine the first time, but remains black on all subsequent opens.
**Root Cause:** `destroyGui()` was invoking `clearGuiEventSources()`, which deleted the `QSocketNotifier` listening to the plugin's X11 file descriptor. Plugins usually register their X11 Display FD only *once* per plugin instance. By deleting the listener, the host stopped forwarding X11 events, leaving the plugin unable to draw.
**Solution:** Do not delete event listeners during GUI destruction. Move `clearGuiEventSources()` to the plugin instance destructor in `clap_host.cpp`.

**Code Snippet (`clap_host_gui.cpp` / `clap_host.cpp`):**

```
// In clap_host_gui.cpp:
void ClapSynthIF::destroyGui() {
  // WE MUST NOT CALL clearGuiEventSources() HERE!
  // ... proceed with gui->destroy()
}

// In clap_host.cpp:
ClapSynthIF::~ClapSynthIF() {
  // Clear events explicitly here before GUI destruction to avoid memory leaks.
  clearGuiEventSources();
  destroyGui();
  // ...
}

```

## Bug 4: Segmentation Fault (SIGSEGV) in Surge XT

**Symptom:** The host crashes immediately upon closing the plugin window.
**Root Cause:** Before calling `gui->destroy()`, the host attempted to explicitly detach the X11 window using `_extGui->set_parent(_plugin, nullptr)`. However, the CLAP specification requires the second argument to be a valid pointer to a `clap_window_t` struct. Passing `nullptr` caused a segmentation fault when the plugin tried to inspect the API type.
**Solution:** Remove the `set_parent(nullptr)` call. A well-behaved CLAP plugin automatically cleans up and destroys its child window when `gui->destroy()` is invoked.

**Code Snippet (`clap_host_gui.cpp`):**

```
void ClapSynthIF::destroyGui() {
  if(_isGuiCreated && _extGui && _plugin) {
    if(_isGuiVisible)
      _extGui->hide(_plugin);

    // Call destroy directly. DO NOT pass nullptr to set_parent().
    _extGui->destroy(_plugin);
  }
  // ...
}


