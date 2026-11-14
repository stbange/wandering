# Observable requirements

## First person and input
The player walks, jumps, pilots and constructs through the same eye camera.
Mouse deltas are integrated independently of dt, in order, with immediate pitch
clamping. Arrow look does not feed walking or steering. Menus and lost focus
pause active time and release the cursor; closing them restores the context.

## Sanctuary and locomotion
Every room is safe. Entering clears harmful exposure. Indoor falls recover
without loss. Attacks in either direction cannot cross the sanctuary. Helm
movement requires occupancy; release brakes. Player attachment survives turning,
translation and jumps. Terrain clearance includes all room corners and legs.
Enemy bodies do not participate in castle collision. Solid scenery does.

## Boarding
Board only when parked. Stairs solve terrain-relative landings, straight then
sideways, grade <= 0.8, flight <= 48m. Reject blocked or unwalkable routes.
Retraction and deployment reject player intersection. Restoring deployed stairs
recreates their support before gravity; the logical parked pose stays exact.

## Economy
Backpack 24, pantry 80, cabinets 40/12, permanent satchel +16. Validate every
ingredient, output capacity, stock, currency and placement before a commit.
Home payment order is backpack, pantry, cabinets by stable room ID. Cancellation,
failure and reward replay preserve all records. Crops grow only in active time,
do not die, and are harvested once. Compost and Careful Tending never stack.

## Growth and progression
First room grows upward, then valid upper sockets spread outward. Preview
reports dimensions, rotation, cost and failures. Preserve furniture contents and
crop/work records or reject replacement. Rest and Save are separate. Rested and
meal effort spend only on outdoor exertion. Peaceful trade and foundry brake
unlock the same essential reserve as combat. Essential growth requires no fight.

## World and persistence
Seed and content versions persist. Seeded features use stable IDs; no reroll on
streaming, negative coordinates or restore. Terrain chunks <=32, enemies <=12,
queues and mutations bounded. Unknown collision prevents movement. Failed
storage halts exploration. Complete saves validate before replacing live state;
invalid, truncated or incompatible files preserve it. Recovery caches preserve
provenance and reject full-backpack retrieval atomically.

## Acceptance evidence
Tests must exercise success and rejection, real collision/raycast paths,
prolonged platform movement, 20/35/45-degree slopes, reverse and turbo, shared
costs, yield replay, corrupted saves and peaceful expansion. Automated gameplay
must drive the delivered application, capture screenshots and inspect failures.
Human and physical XR acceptance remain distinct from automated checks.
