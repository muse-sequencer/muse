



### In CLAP the descriptor has two distinct fields: 

 - `id` is the stable, machine-readable identifier (the reverse-DNS `org.surge-synth-team.surge-xt`), and 

 - `name` is the developer-set human display string ("Surge XT"). Neither is translated. 



### In MusE's model:

 - `_label` (and `_uri`) is the **stable key**: it's what `find()` matches on and what gets written into song files for re-loading. It must not change between sessions/versions or old songs stop matching.

 - `_name` is the **display string**.

 - `label` is the durable identifier.


The plugin scanner already has it right: 
id → `label`/`uri` (stable key), `name` → `_name` (display). 


---

### Considerations 

Setting `_label = desc->name` would make the *storage/matching key* the display name — the opposite of its purpose. 

It mostly "works" today only because `find()` prefers `_uri` when it's non-empty (plugin_list.cpp), so label is effectively cosmetic for CLAP — but you'd be relying on that, conflating two roles, losing the id in the plugin manager, and risking collisions if two plugins share a name. It also needs a cache rebuild and could trip up older songs that stored the id as label.

The reason `label()` looks fine for LADSPA/LV2 is that their `Label`/short-name happens to be short and readable ("amp"), while CLAP's formal id is a reverse-DNS string. That's a cosmetic quirk of the shared namer at synth.cpp:361 using `label()` — the value that's *meant* to be the stable key.


### clean choice:

1. **The namer (recommended, generic):** the one-liner from last turn — `createSynthInstance()` builds the instance name from `name()` instead of `label()`. Fixes every plugin format consistently and keeps `label`/`uri` as the stable id for matching and persistence. This is the semantically right place.


### alternative: 

2. **If you specifically want to avoid touching the shared namer**, override `name()`/`displayName()` on `ClapSynth` so only CLAP's display string changes, leaving `label()` (the id) intact for storage. This keeps the change localized to CLAP without corrupting the stable key.

Either keeps `label = desc->id`. I'd avoid `label = desc->name`: it fixes the symptom by breaking the contract that `label` is the durable identifier.

If you still prefer the scanner route knowing the tradeoff, it's a one-liner in `plugin_cache_writer_clap.cpp` (`info._label = PLUGIN_SET_CSTRING(desc->name);` + rescan) — but I'd recommend option 1 instead.






