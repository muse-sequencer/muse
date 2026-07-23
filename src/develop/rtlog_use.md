

### rtlog.h - debug prints from realtime-threads. 


example: 

```cpp
#include "rtlog.h"

// Anywhere on a real-time thread (audio callback, RT-context message sender, etc.):
void JackAudioDevice::someRtFunction(int channel, float value)
{
  if(value > 1.0f)
  {
    // RT-safe: never blocks, never locks, never allocates. Formats into a
    // fixed-size slot and queues it - the actual fprintf happens later,
    // off this thread, via MusE::heartBeat() -> rtLogFlush().
    MusECore::rtLog("channel:%d clipping, value:%f", channel, value);
  }
}
```

Same `printf`-style format string and varargs as `fprintf` — just swap the call and drop the trailing `\n` (added for you in `rtLogFlush()`). No `#include <cstdio>` needed for this, no `stderr` handle, no gating required for RT-safety (though you can still wrap it in `if(MusEGlobal::debugMsg)` if it's a verbose/noisy message rather than a hard error, same as we did in `wavetrack.cpp`).

One thing to keep in mind: don't build the string yourself first (e.g. `QString::arg(...)` then pass `.toUtf8().constData()`) — that defeats the purpose, since `QString` formatting can allocate. Pass the raw values and let `rtLog`'s internal `vsnprintf` do the formatting.



### flush

rtLogFlush() itself is only called from one place — MusE::heartBeat() in app.cpp:Show 
the call site of rtLogFlush in app.cppShow the call site of rtLogFlush in app.cpp

So the full chain: RT thread calls rtLog() → queues into the ring buffer in rtlog.cpp → sits there until MusE::heartBeat() (a Qt slot on the GUI thread, driven by MusEGlobal::heartBeatTimer, firing at 1000/guiRefresh ms) calls rtLogFlush() 

→ which does the actual fprintf(stderr, ...), **on the GUI thread**, not the RT thread.

