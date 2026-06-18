
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



Auf einer reinen Betriebssystem-Ebene: 
Ein `hide()` (unter X11 ein `Unmap`) zerstört das Fenster nicht, es wird nur unsichtbar gemacht.

Dass das Plugin beim erneuten Anzeigen (`Map`) trotzdem einen schwarzen Bildschirm zeigt, liegt an der Kombination aus **X11-Reparenting (Einbetten)** und der Art, wie moderne **Plugin-UI-Frameworks (wie JUCE, VSTGUI oder IPlug2)** rendern.

Hier sind die drei Hauptgründe, warum der Kontext bei Plugins in diesem Szenario oft verloren geht oder kaputtgeht:

**1. Hardwarebeschleunigung (OpenGL / Vulkan)**
Moderne Plugins nutzen für ihre GUIs oft die Grafikkarte. Wenn ein Host-Fenster (Qt) unter X11 unsichtbar gemacht wird (`Unmap`), wird das eingebettete Plugin-Fenster mitgerissen. Dabei invalidiert der X-Server oder der Grafiktreiber häufig die daran gebundene "Swapchain" (die Bildpuffer für OpenGL/Vulkan) oder den Surface-Context. Wenn das Fenster wieder angezeigt wird, rendert das Plugin oft fleißig weiter in den alten, nun ungültigen Puffer ("Dead Surface") oder der Treiber verweigert das Rendern, was zu einem rein schwarzen Bildschirm führt.

**2. Schlechtes Event-Handling in den Plugins**
Wenn das Fenster unsichtbar wird, feuert der X11-Server ein `UnmapNotify`-Event. Beim erneuten Anzeigen kommt ein `MapNotify` und ein `Expose`-Event (die Aufforderung: "Zeichne dich neu!"). Viele Plugin-Frameworks fangen diese Events bei *eingebetteten* (reparented) Fenstern nicht sauber ab. Das Plugin denkt intern "Ich bin ja noch da und initialisiert", ignoriert das `Expose`-Event und weigert sich, seine UI-Elemente neu aufzubauen.

**3. Lebenszyklus-Diskrepanz zwischen Host und Plugin**
Wenn der Host (MusE) das Eltern-Fenster (`_editorWindow`) einfach via Qt versteckt, ändert sich der Zustand auf X11-Ebene, aber das Plugin erfährt über die CLAP/VST-API nicht zwingend, dass es aufhören soll, Frames zu generieren. Timers und File-Deskriptoren (wie X11-Events) laufen weiter, laufen aber ins Leere. Das führt zu desynchronisierten Zuständen.

**Warum `destroy()` der Industrie-Standard unter Linux ist:**
Weil der DAW-Entwickler (Host) keinen Einfluss auf den unsauberen Code hunderter verschiedener Plugins hat, haben sich DAWs unter Linux (wie Bitwig Studio, Reaper, Ardour) darauf geeinigt, sehr defensiv vorzugehen. Es hat sich in der Praxis als wesentlich robuster erwiesen, die GUI bei einem "Schließen" des Fensters komplett via `gui->destroy()` aus dem Speicher zu werfen und die Ressourcen freizugeben. Bei einem Klick auf "Anzeigen" wird sie frisch via `gui->create()` initialisiert. Das dauert zwar ein paar Millisekunden länger, garantiert aber, dass das Plugin seinen Grafik-Kontext und seine internen State-Machines zu 100% sauber neu aufbaut.



