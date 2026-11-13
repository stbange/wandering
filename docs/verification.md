# Evidence register

## Native foundation evidence — 2026-11-13

Environment: Windows x64, w64devkit GCC16.2, CMake4.5.0, SDL3.4.20,
Lua5.4.8, real OpenGL3.3 context. All commands ran against this source tree.

- `openspec validate --all --strict --no-interactive`: foundation change passed.
- `ctest --test-dir build --output-on-failure`: core and executable game tests passed.
- Coverage build + `python tools/coverage.py build-core --gcov .deps/w64devkit/bin/gcov.exe`:
  **347/348 authored core lines (99.71%)**, **513/636 branches (80.66%)**.
  Denominator is `core.c`, `save.c`, `content.c`; excludes SDL/GL adapters and dependencies.
- Actual `SDL_PushEvent` path: walk cabin/ramp, aimed helm, W/D/Shift turbo,
  attached stability, jump while piloting, arrow look, release braking, cleared
  held keys, pause, focus loss/recovery, F5/F9, help preference and controls after
  restore. Straight entrance walked out and back without jump.
- Core tests include one minute of carried turning at turbo; 20/35/45-degree
  analytic slope traversal in both directions; pause/growth; full-inventory
  transactional failures; repeat harvest/gather rejection; same-seed terrain
  seam queries and corruption of every byte in a serialized save.
- `build/game-home.bmp` and `build/game-exterior.bmp`: inspected actual rendered
  interior/exterior. Furniture, window views, helm, six legs, roof and stairs visible.
- Initial game-test failure: route ended on the command ramp, then aiming from
  deck height missed above the helm. Test now walks onto the deck and explicitly
  looks down at the helm. Assertions retained; final run passes.

**Human playtest acceptance: pending.** No comfort, macOS, Linux, browser,
mobile, XR or full-game acceptance is inferred from these tests.

## Remaining gaps

The deliverable is a foundation with an early home loop, not milestones A–G
complete. In particular: no room construction/replacement, cabinets, persistent
streaming/chunk storage, three biomes, solid outdoor obstacle collision, combat,
traders, recovery caches, knowledge/techniques, cooking, compost/hoe gating,
foundry, keep/mine/hamlet destinations, audio, cutout sprites, full night lighting,
dynamic shadowing, turning stair routes or WebXR. Terrain renders in a bounded
grid around the camera and uses double logical coordinates; it is not the required
integer-chunk streamed world. Only straight stairs are supported. Recipe quantities and crop tuning now load from Lua; home action dispatch uses
content/rules.lua and retains C validation.
Web build configuration/touch adapter exist but are unverified; durable browser
saving is incomplete. Native saves cover implemented state only, with format1.

Progress against every full requirement is preserved in `requirements.md`;
unchecked entries are not silently dropped.

## Focused first playable acceptance checklist

- [ ] Walk the furnished starter, collide with walls and furnishings, jump.
- [ ] Aim at helm and press E. Steer/turn/reverse/turbo; release to stop.
- [ ] Check free look at varied headings/pitches with both mouse and arrows.
- [ ] Observe stable indoor motion and jumps during extended platform travel.
- [ ] Pause/resume, switch focus, hide help, save/restore and regain controls.
- [ ] Inspect low window views, lighting, physical helm and human scale.
- [ ] Review measured automated evidence separately from these comfort judgments.

## Scripted-home follow-up and environment interruption

Before disconnection on 2026-11-13, the scripted-home core and SDL game tests
passed on Windows. Core coverage at that checkpoint was **404/405 lines (99.75%)**
and **585/738 branches (79.27%)**, with the same three-file denominator.
A later furniture-support fix and expanded real planter/workshop event tests
also passed the core and executable tests; coverage was not rerun afterward.

The earlier package, including external rules and licenses, was extracted into
a fresh temporary directory and passed its real input/render test while launched
from a different working directory. That package is on the disconnected Windows
host and cannot currently be attached from this environment.

The source of this follow-up was reconstructed from the session edits after the
execution host became unavailable. Cross-platform CI now rechecks that exact
reconstructed source. CI configuration alone is not a passing result.
The original tested foundation is commit 3221b410b2f908c22cac39edad2027f81a42dae7.

Human acceptance remains pending. Browser saving, mobile usability and WebXR are
not released. The ground-level bell still needs an interaction path. A partial
local bell edit was excluded from reconstruction because it had not compiled.
