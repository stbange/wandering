# Design

The core has no SDL, GL, file I/O or Lua dependency. Fixed-step simulation and
commands operate on a bounded, serializable state. Commands validate against a
copy and commit exactly once; inventory consumption also updates the expedition
provenance ledger. Stable content IDs are shared with Lua and saves. No renderer
or scene rebuild grants items. Menus stop simulation rather than advancing a
second clock.

Positions use signed 64-bit chunk coordinates and normalized local offsets.
The renderer uses a nearby origin. Terrain is continuous seeded value noise;
starter approaches are flattened. Active terrain and encounters are bounded.
Persistent mutation capacity is explicit: exhaustion refuses the mutation and
travel, with feedback, instead of silently evicting records.

SDL3 owns events, relative mouse, touch, focus and GL context creation. One
OpenGL ES 3 / desktop GL 3 shader path renders textured low-poly meshes and a
bitmap HUD. Original geometry, palette, textures and audio are external assets.
Lua supplies items, recipes, crops, prices and tuning; native checks remain
authoritative. Browser storage is flushed to IDBFS; native saves use checksums,
validation, temporary files and a previous generation.

Castles use a room graph and local player attachment. Moving the castle changes
one transform, never adds the same platform displacement to the player twice.
The cabin remains level above the highest sampled rotated footprint. A parked
position does not drift to a newly sampled ground height. Stairs are explicit
walkable segments, built before a restored player resumes simulation.
