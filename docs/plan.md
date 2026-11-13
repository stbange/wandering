# Implementation and acceptance plan

| Step | Deliverable | Implementation | Automated verification | Human acceptance |
|---|---|---|---|---|
| 0 | Repository, licenses, OpenSpec, architecture and traceability | Complete | Strict OpenSpec pass | Pending |
| A | First-person moving-home slice; input, collision, Lua content | Implemented; Windows packages available | Core + SDL gameplay and cross-platform CI pass | Pending |
| B | Helm, sanctuary, slopes, adaptive boarding, early save/restore | Partial: helm, straight stairs, saves | Core slope matrix and stair traversal pass | Pending |
| C | Authoritative inventories, gardening, recipes, day/night, full HUD | Partial: supply/garden/satchel loop | Atomic failure and growth tests pass | Pending |
| D | Seeded biomes, bounded streaming, long-distance precision | Pending | Pending | Pending |
| E | Upward-first furnished rooms and replacement | Pending | Pending | Pending |
| F | Peaceful trade, combat, progression, preparations, foundry | Pending | Pending | Pending |
| G | Generated destinations, complete persistence, all target packaging | Desktop/web build paths; gameplay scope pending | CI builds; browser/XR runtime pending | Pending |

Each step: propose observable OpenSpec scenarios, implement a runnable increment,
run meaningful unit and gameplay tests, inspect visuals, record evidence, commit
and push. Unchecked items remain explicit. No automated run grants acceptance.

## Verification policy

- At least **95% line coverage of authored authoritative C core** in the unit
  suite, measured by gcov. Report denominator and branch coverage explicitly.
- SDL/GL platform code has separate actual event-path and rendered game tests;
  core coverage must never be represented as whole-application coverage.
- Lua definitions are exercised through the actual loader and command paths.
- CI builds Windows, Linux, macOS and Emscripten; a configured job is not evidence
  of a passing build. WebXR requires an actual headset/browser acceptance pass.
- Coverage cannot replace collision, save recovery, bounded-memory and visual tests.

## Narrative decision

The user supplied two premises. The detailed 18-section Hearthstrider contract
is the implementation baseline. A sentient rural van and nightly multiversal
journeys are a possible narrative variant requiring a later product decision;
do not silently replace the walking castle or claim a van opening is implemented.
