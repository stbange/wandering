# Current feature status

The complete requested game remains in docs/requirements.md. This table describes
the playable foundation actually delivered. Human acceptance is pending throughout.

| Feature | Implementation | Verification |
|---|---|---|
| C17 engine, SDL3, OpenGL3.3/GLES3 renderer | Implemented | Native and Emscripten compilation |
| First-person moving home and manual helm | Implemented | Core invariants plus real SDL events |
| Mouse/arrow look, jumping, pause/focus/restore | Implemented | Core and rendered gameplay tests |
| Starter 12x16m home, furniture, six legs | Implemented baseline | Windows screenshots inspected |
| Terrain-following cabin | Implemented baseline | Analytic 20/35/45-degree slopes |
| Straight deployed stairs and interlocks | Implemented baseline | Two-way walking without jump |
| Shared backpack/pantry transactions | Implemented | Rejection and repeat-action tests |
| Two crops and active-time growth | Implemented baseline | Unit and real activity-panel tests |
| Planks, twine and permanent satchel | Implemented | Lua definitions and atomic C commands |
| Editable Lua activity logic | Implemented | Rule-edit, invalid-command and loop tests |
| Versioned saves and previous generation | Implemented for current state | Corruption, F5/F9 and recovery tests |
| Original external art and MIT/CC BY licenses | Included | Package loading, license inventory |
| Windows package | Produced | Local relocated package passed; CI builds |
| Linux package | Produced | CI core and rendered game tests |
| macOS package | Produced | CI build/core; runtime playtest pending |
| Emscripten package | Produced | Build only; browser runtime pending |
| Mobile touch adapter | Initial implementation | Device/runtime verification pending |
| WebXR | Not implemented | Pending |
| Full adaptive/turning stairs and entrance bell | Incomplete | Pending |
| Full chunk streaming, three biomes, world mutations | Not implemented | Pending |
| Room construction/replacement and cabinets | Not implemented | Pending |
| Combat, trade, techniques, meals and recovery caches | Not implemented | Pending |
| Foundry, keeps, mines and hamlets | Not implemented | Pending |

Core coverage is explicitly scoped to core.c, save.c and content.c. Rendering,
platform/input adapters and dependencies are excluded from that percentage and
have separate integration checks. Coverage does not establish human acceptance.
