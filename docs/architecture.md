# Extending the game

`src/core` owns authoritative state and commands. It has no SDL, OpenGL, Lua or
filesystem dependency. `wc_step` advances active time; a paused or storage-failed
game advances nothing. Commands validate a candidate state and publish it only
after the entire operation succeeds. Rendering never grants inventory or rewards.

`src/main.c` maps SDL events into separate on-foot, helm, construction and panel
contexts. Key state is owned by the application and updated in event order;
focus changes cannot erase a later key-down from the same event batch. Mouse
deltas are radians, not velocities multiplied by frame time. `src/render.c`
draws a nearby origin, external art, a 32-entry terrain cache and the first-person
HUD. The fixed terrain generator supplies collision immediately, so there is
no interval with a visible chunk and missing collision. Generation is currently
synchronous and cached; asynchronous workers are future optimization work.

`assets/scripts/content.lua` defines prices, recipes, equipment, reward bundles,
crop timing and room costs. Preserve item IDs when editing existing saves.
`assets/scripts/home.lua` composes original geometry and the palette using Lua.
The native loader exposes base/math libraries without OS, network or file APIs.
Bad scripts retain the old definition object and report an error. New command
kinds and item IDs require a C schema change; this is not an unrestricted mod
runtime. New recipes can use the existing item slots up to the recipe limit.

Shaders, a 32x16 RGBA atlas, bitmap font and WAV audio live in `assets`. Native
builds copy that directory beside the executable. Web builds fetch the same
files before starting WASM: editing Lua, shaders or art does not require baking
them into the executable. `tools/generate_art.py` reproduces the original atlas,
font and chime. After editing source assets, copy the folder to the build (or
rebuild the executable target). `--assets PATH` or `WANDERING_ASSETS` overrides
the native directory.

Positions have signed 64-bit chunk coordinates and normalized local offsets.
Terrain and site placement derive only from the saved seed and stable IDs.
Destination foundations reserve their approaches before resources and trees
are attached. Authored solid boxes are shared by exterior movement, interaction
occlusion and the renderer. Room geometry derives from the same graph used by
floors, stairs, sanctuary and travel clearance.

The mutation journal retains up to 2,048 records, and recovery storage retains
16 caches. Reaching a journal limit freezes exploration safely. Terrain travel
is effectively unbounded, but indefinitely accumulating persistent changes
requires the planned disk-backed journal paging. No worker queues or mutation
records are silently discarded. This limit is a known gap against the full
infinite-journey brief, not a completed streaming-persistence milestone.

Save encoding is explicit little-endian, checked and versioned, independent of
C structure padding. Restore validates into a separate state and only then
publishes it, parked and on foot, with serialized support already present.
Native publication flushes a temporary file, validates it, retains the verified
previous generation and renames the new generation. Web storage uses IDBFS;
failed synchronization freezes simulation and leaves export available.

Add an OpenSpec change, observable success/rejection cases, core tests and
gameplay evidence before extending behavior. See `docs/verification.md` for
what has actually been tested and what still needs acceptance.
