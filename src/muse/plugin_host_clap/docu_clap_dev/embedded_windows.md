
**Embeddable GUI** means the plugin draws its interface inside a parent window provided by the host (MusE), looking like a native part of the application.

**Floatable GUI** means the plugin creates and manages its own independent desktop window, which is often used as a fallback on systems like Wayland where direct embedding doesn't work.


Native **Wayland's** strict security and isolation model intentionally prevents traditional cross-process window embedding (known as "reparenting" in X11).

In the older **X11 system**, a host application could simply hand over a **Window ID** to a plugin, and the plugin could draw its interface directly inside that host window. Wayland treats every application's graphical surface as isolated and secure, so a plugin process cannot arbitrarily draw inside the host process's surface.

---

In the `clap_host_gui.cpp` file, you can see this logic specifically handled:

1. When MusE runs natively under Wayland, `_editorWindow->winId()` returns a Wayland surface pointer, not an X11 Window ID.
2. If the host passes this to a plugin that expects an X11 Window ID (`CLAP_WINDOW_API_X11`), the plugin fails to attach, resulting in an empty window.
3. To bypass this, the code checks `if(isWayland)` and forces the plugin to use a **"floating"** window (a completely separate, independent window that the plugin manages itself) if the plugin supports it.

If a user really wants embedded plugin windows on a Wayland desktop, they have to force the host application to run through **XWayland** (the X11 compatibility layer) by launching it with an environment variable like `QT_QPA_PLATFORM=xcb`.




===============
===============




On a pure operating‑system level:  
A `hide()` (an `Unmap` under X11) does not destroy the window; it only makes it invisible.

The reason the plugin still shows a black screen when it is shown again (`Map`) is the combination of **X11 reparenting (embedding)** and the way modern **plugin UI frameworks (such as JUCE, VSTGUI, or IPlug2)** render.

Here are the three main reasons why the context is often lost or broken for plugins in this scenario:

**1. Hardware acceleration (OpenGL / Vulkan)**  
Modern plugins often use the GPU for their GUIs. When a host window (Qt) is made invisible under X11 (`Unmap`), the embedded plugin window is affected as well. In this process, the X server or the graphics driver frequently invalidates the associated “swapchain” (the image buffers for OpenGL/Vulkan) or the surface context. When the window is shown again, the plugin often continues rendering into the old, now invalid buffer (“dead surface”), or the driver refuses to render at all — resulting in a completely black screen.

**2. Poor event handling inside plugins**  
When the window becomes invisible, the X11 server fires an `UnmapNotify` event. When it becomes visible again, a `MapNotify` and an `Expose` event follow (the request: “Redraw yourself!”). Many plugin frameworks do not correctly handle these events for *embedded* (reparented) windows. The plugin internally thinks “I’m still here and initialized,” ignores the `Expose` event, and refuses to rebuild its UI elements.

**3. Lifecycle mismatch between host and plugin**  
When the host (MusE) simply hides the parent window (`_editorWindow`) via Qt, the state changes on the X11 level, but the plugin does not necessarily receive a CLAP/VST API notification telling it to stop generating frames. Timers and file descriptors (such as X11 events) continue running but lead nowhere. This results in desynchronized internal states.

**Why instead close() mostly `destroy()` is used for integrated plugin-GUI :**  

Because the DAW developer (host) has no control over the messy code of hundreds of different plugins, Linux DAWs (such as Bitwig Studio, Reaper, Ardour) have agreed to take a very defensive approach. In practice, it has proven far more robust to completely remove the GUI from memory via `gui->destroy()` when the window is “closed,” and release all resources. When the user clicks “Show,” the GUI is freshly initialized via `gui->create()`. This takes a few milliseconds longer but guarantees that the plugin rebuilds its graphics context and internal state machines 100% cleanly.

---

