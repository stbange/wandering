# Verification record — first playable release

Baseline: `stbange/wandering`, main, `570fd719ea0dc9751261e8e80d683b5966cbb17e`.
Recorded on 2026-11-14. Human acceptance is pending throughout this table.
Evidence JSON and selected screenshots are in `docs/evidence` and
`docs/screenshots`; full logs, HTML coverage and captures are generated in
`artifacts/`. CI rebuilds all supported native targets and the web version.

| Area | Implementation | Automated evidence | Human/device acceptance |
|---|---|---|---|
| C17 / SDL3 / GL / Lua | Native and WebGL2 app, external assets | Linux build/render and Emscripten build | Windows/macOS and audio devices pending |
| First-person controls | Ordered mouse/key events, contexts, help, pause/focus/load | Arrow isolation, focus recovery, F9, mobile input | Mouse feel and physical mobile pending |
| Sanctuary / helm | Safe room graph, braking, enemy-independent castle travel | Sanctuary, defeat, attachment and turbo tests | Long manual journey pending |
| Slopes / legs / clearance | Level cabin, independently grounded six legs, expanded footprint | Shared steering/clearance path on 20°,35°,45° plane courses; reverse/turn/turbo | Natural steep-terrain visual acceptance pending |
| Adaptive boarding | Straight/turning flights, grade and landing checks, persistent support | Higher/lower/cross-slope solver; actual bidirectional stair walk; restore on stairs | Complex physical obstructions need broader acceptance |
| Economy | Candidate-state commit, shared payment order, prices and stocks | Success/rejection, capacity-after-inputs, repeated rewards, transaction fuzz | Balance pending |
| Gardening | Two crops, active time, mature persistence, non-stacking treatment | Plant/treat/harvest failures and pause; browser homecoming harvest | Presentation/balance pending |
| Rooms / furniture | Upward-first halls, higher reach, rotation, furnished rooms, cabinet packing | Twelve-room graph, upper-floor restore, preservation and rejection | Broad topology/access visual acceptance pending |
| Physiology / techniques | Rest, meals, exhaustion, XP cap, three techniques, spear/workshop | Effort costs, gating, repeat protection, peaceful alloy trade | Combat feel pending |
| Foundry | Notes, plate, steam, brake, guardian recovery/core, one-time rewards | Peaceful full application journey; guardian core tests | Manual combat/telegraph acceptance pending |
| World | Seeded smooth terrain, names, foundations, biomes, origin-relative render, 32 cached chunks | Negative/distant seams, reproducibility, site reservations and budgets | Extended travel and landscape variety pending |
| Destinations | Basic keep/mine/hamlet geometry with clues, rewards and solid collision | All families generated, flat reserved approaches, shared obstacle boxes | Richer authored wings, archive, mine branches and house content remain incomplete |
| Persistence | Version/checksum/validation, previous generation, browser IDBFS/export | Corrupt/truncated/forged payloads, fallback, failed storage freeze, complete browser restore | Abrupt power-loss/device persistence acceptance pending |
| WebXR | Per-eye GL render, XR frame clock, head aim, controller input and panel navigation | Synthetic pose/per-eye/rotation integration | Physical headset untested |

The latest local core run passes **37,387 checks**, with **97.8% executable-line
coverage**, **100% function coverage** and **87.0% branch coverage** across
`src/core`. CI enforces at least 95% core line coverage. SDL, Lua-loading,
filesystem and renderer glue are outside that metric and have integration
checks; this is not a claim of 95% whole-application or branch coverage.

The full application journey passes **42 assertions** using keyboard, pointer
and the public touch-look input. It crafts from shared supplies, builds an upper
hall, plants both crops, rests, pilots with turbo, saves/restores, walks stairs,
gathers, trades for alloy, enters the foundry through real openings, engages the
peaceful brake, claims the reserve, returns by the stairs, deposits supplies,
harvests and restores the complete journey. A separate **15-check** suite covers
input contexts, focus, mobile touch, corrupt-file fallback, storage failure and
synthetic XR integration. Runtime-error collections are empty.

AddressSanitizer and UndefinedBehaviorSanitizer pass the core/content suite.
LeakSanitizer is disabled in the managed environment because its process
inspection is unavailable; no leak-scan result is claimed. Native SDL3/Mesa
rendering was captured using the offscreen video driver, independently of WebGL.

Remaining scope against the complete brief: disk-backed mutation paging beyond
the bounded 2,048-record journal / 16 recovery caches; asynchronous generation
workers; richer authored destination interiors and seeded orientation; collision
for every decorative indoor furnishing; and broad human/device/platform
acceptance. Enemy presentation is an initial telegraph/recovery implementation,
not a finished pursuit/animation system. These are explicit follow-up milestones.

## Separate Pages editions

The publishing package adds distinct standard and WebXR builds. Local testing
passes 34 Pages checks (both routes, external assets, WASM MIME type, edition
links, touch controls and XR separation), the 15-check input suite on the staged
WebXR page, and all 42 journey assertions on the staged standard page. The
deployment workflow is gated by native, coverage and browser verification.
Public URL acceptance is pending repository Pages activation; see
[publishing](publishing.md).
