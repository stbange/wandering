# Product contract and feature checklist

Status on creation: **all gameplay requirements pending**. This file preserves
the required scope; milestone evidence records what a runnable build actually does.

## 1. Experience

Single player, exclusively first person, sole occupant of a growing furnished
walking home. Prepare → steer → park → explore/gather/trade/optional fight →
return → garden/craft/expand. Essential progression has a peaceful route.
No crew, multiplayer, siege, structural destruction, fuel, starvation, dying
plants or mandatory maintenance. Region entry never requires player power.

## 2. Sanctuary and controls

- [ ] All rooms are safe, enemies cannot enter, injure indoor players or block
  travel, neither side attacks across sanctuary; entry clears danger/status.
- [ ] Indoor falls recover without loss. Gathering, trade and combat require exit.
- [ ] WASD walk; mouse incremental look; arrows alternative look only; Space jump;
  Shift sprint or helm 5× turbo; E aimed interaction and enter/release helm.
- [ ] Helm W/S forward/reverse, A/D turn; mouse/arrows freely look.
- [ ] I equipment; B build; R rotate; F furniture; LMB/X melee; Q rewardless ward;
  K techniques; V Second Wind; C Power Strike; Escape close/pause; F1 help;
  Tab help in gameplay / menu navigation; F5 save; F9 restore.
- [ ] Separate gameplay, helm, construction and menu contexts. Pitch clamp drops
  overshoot; look independent of heading, pitch, frame rate/event grouping.
- [ ] Menus pause with the first-person view retained and cursor released.
  Focus loss pauses/releases; resume/restore clear stale input and recover control.
- [ ] Crosshair and short interaction ray: door, bell, instruments, storage,
  bed, planter, workstation, trader, clue and loot. Visible help toggle persists
  through session pause/restore; hidden help retains stats and interactions.

## 3. Hearthstrider

- [ ] Six articulated legs, large feet, exposed joints/pistons, tapered hull;
  timber/stone/metal, copper roofs, chimney, lookout and supported upper wings.
- [ ] Sheltered rear oak double gate, lanterns, folding stairs/rails; framed
  windows, low sills and reachable surfaces. Person 1.8m, eye 1.62m.
- [ ] Starter 12×16m: raised helm/map deck, workshop, bed, pantry, two planters.
- [ ] Level cabin, independent ground-reaching legs, whole rotated expanded
  footprint clears terrain. Travel up/down/reverse/turn on 20°,35°,45° incl turbo.
- [ ] Speed 5m/s, turbo 25m/s, turn ~0.38rad/s. Solid body/hull/leg obstacles
  block; enemies do not. Manual helm occupancy required; release brakes.
- [ ] Stable indoor translation/turn/jump/uphill; apply platform motion once
  and retain mouse orientation. Automatic steering is future work.

## 4. Boarding

- [ ] Parked-only enter/exit, adapt stairs to higher/lower/cross-sloping terrain.
  Straight then turning platform/side flight; up to ~48m, grade ≤0.8.
- [ ] Visible decking, collision, handrails, landing and walkable thresholds agree.
  Ground bell reopens gate. Test two-way walking without jumping.
- [ ] No deployment through player or retraction under them; include door,
  flights, turn platform and space beneath stowed stairs in interlock checks.
- [ ] Reject missing terrain, obstructions and unwalkable landings with feedback.
  Parked logical pose never drifts. Restore deployed support before gravity.

## 5. Supplies and home work

- [ ] Atomic authoritative inventory: pack24, pantry80, satchel +16 permanent,
  standard cabinet40, compact12. Full/cancelled/invalid/repeated actions cannot
  pay or reward partially. Validate inputs/output capacity/stock/coins/placement.
- [ ] Home costs consume pack, pantry, cabinets in stable-ID order. Every cost
  shows available/required and paying pool. Crafted output goes to pack, checking
  space after consumed inputs. All definitions data driven.
- [ ] Outdoor timber/stone/fiber/seeds; depletion persists, replenish ~120 active s.
- [ ] Recipes: 2 timber→2 planks; 3 fiber→2 twine; sunroot+mint+stone→2 compost;
  2 planks+2 twine→satchel; discovered wedge costs2 timber+stone.
- [ ] Reusable hoe. Sunroot45s and mint60s, each yields3 crop+replacement seed.
  Active time only even with inactive rooms; ripe forever. Compost OR Careful
  Tending halves duration once; no stacking. Plant/treat/harvest atomic.

## 6. Rooms and furniture

- [ ] Whole furnished rooms, first-person sockets, upward first: safe roof opening
  and continuous stairs to upper hall; lateral annexes only upper tiers.
- [ ] Hall12×16m, annex6×6m; higher wider; envelope38×24×18m and max12 physical
  rooms, additionally valid topology. Each hall/annex costs6 planks+2 twine+4 stone.
- [ ] Preview size/rotation/cost/validity. Reject overlap, blocked passages,
  occupied stair holes, terrain conflict, unsupported sizes and missing resources.
- [ ] Furniture returns packed old item, preserves contents/crops/work/access,
  rejects unsafe replacement, reuses packed items before raw materials.
- [ ] Roof/braces/collision/sanctuary/travel clearance follow complete graph.
  Rebuild/load never regrants furnishings or contents.

## 7. Progression

- [ ] HP100/stamina100; outdoor sprint18/s, attack8, recovery delay0.8s,
  recovery14/s or rested20/s. Exhaustion preserves walking/home work/piloting;
  sprint needs release and ≥20 stamina. Gear has no character-level equip gate.
- [ ] Bed separate Rest/Save; instant refill +120 rested effort-s, no time/cost.
  Hearth2 planks+2 stone. Stew2 sunroot+mint gives180 effort-s, costs30% less
  stamina. Only outdoor exertion consumes benefits. No stacking/wasted food.
- [ ] Meaningful discovery/productive achievements only, once. 1 point/12 XP,
  max3 total. Failure/cancel/repeat earns none.
- [ ] Second Wind outdoor spends15 rested-s→up to40 stamina. Power Strike doubles
  weapon damage, costs25 before meal, shared1s cooldown. Careful Tending as above.
- [ ] Foundry or trade unlocks tempered workshop/spear and alloy. Workshop costs
  2 alloy+4 planks+4 stone, spear2 alloy+2 planks+twine, damage28/power5.
  Facilities install once; duplicate rooms cannot multiply permanent bonuses.

## 8. World

- [ ] Smooth streamed effectively infinite world; new random seed once, displayed
  and saved. Explicit seed reproduces. Fixed starting128×128m square centered
  on initial castle with supplies/trader/expansion access/first foundry.
- [ ] Beyond starter: seed+integer chunk coords+small offsets+generation/content
  versions+stable IDs. Floating origin/equivalent. Negative/distant seams correct.
- [ ] Meadows/woodland/highlands; large mountains/ridges/valleys; blended edges,
  believable vegetation; non-colliding bounded distant horizon.
- [ ] Fixed regional danger/suggested power/readiness, no player scaling or entry
  gate. Chunk128m; max32 resident+retiring chunks, max12 prowlers; bounded worker
  queues. Off-thread data where appropriate, safe scene/collision attachment.
- [ ] Block movement into missing collision. Dirty state persists before unload;
  storage failure stops exploration safely. Discoveries/depletion/stock/traps/
  claims survive unload/origin/load. New seeds preserve identical starter.

## 9. Outside encounters

- [ ] Readable warnings/attacks/recovery; aim/range/occlusion/cooldown enforced.
  Outdoor Q only repels eligible prowlers without reward.
- [ ] Atomic merchant items/stock/coins; peaceful gather/garden/craft economy
  reaches expansion and useful equipment.
- [ ] Defeat returns safely to bed, resets vitals/temporary preparation, protects
  knowledge/techniques/gear/coins/home. Cache only newly gathered/looted ordinary
  carried resources since departure. Home-brought, purchased, recovered protected.
- [ ] Home secures haul; consume/sell/deposit update provenance ledger.
  Recovery persistent/atomic with full-pack rejection.

## 10. Destinations

- [ ] Silent Foundry near(-30,28): notes teach wedge/improvements; warned pressure
  spikes, escaping/jumping avoids; one wedge permanently disables; one-time salvage.
- [ ] Deeper steam grates, reachable valve, guardian warning/slam/recovery core.
  Marked maintenance path/brake peacefully unlocks same one-time reserve.
  Route board offers return with haul or continue.
- [ ] Beyond starter: persistent small expeditions; ~60m keep with courtyard,
  furnished wings/great hall/ramped archive; ~60m covered branching adit with
  supports/stores/return markings; hamlet with4 furnished enterable houses.
- [ ] Seeded names/orientation/variation, suitable terrain, reserve approach and
  interior before foliage/resources/enemies. Reachable clues/rewards/escape.

## 11. Presentation and persistence

- [ ] Original retro low-poly meshes/pixel textures, cutout vegetation, smooth
  terrain, colorful day, sun/clouds/sunset/moon/stars/dark night; warm home/lantern.
  Saved active clock,12-minute day starting09:00; pause/load/origin preserve phase.
- [ ] Live HUD numbers/bars: HP/stamina, preparations, XP/points, coins, effective
  damage/armor, pack use, region/power/readiness, castle speed/heading/park/turbo,
  contextual interaction and failures.
- [ ] Versioned validated atomic saves with previous generation. Full journey:
  seed/versions/poses/attachment/rooms/furniture/inventories/crops/work/clock/
  vitals/buffs/gear/techniques/facilities/XP/coins/discoveries/stock/depletion/
  enemy cooldowns/mechanisms/claims/caches/entrance intent.
- [ ] Corruption/incompatibility leaves live state intact and reports. Controlled
  restore rebuilds terrain/upper rooms/interiors/stairs first; parked on-foot
  response without freezes/falls/duplicates. Confirm New Journey/new seed; no
  overwriting save until explicit Save.

## 12. Platforms, verification and future boundaries

- [ ] C core+editable Lua, external graphics/sounds/scripts, Windows/Linux/macOS,
  Emscripten web with automatic mobile touch controls; WebXR presentation.
- [ ] MIT code, CC BY4 original art, setup/build/test instructions, playable
  Windows package, OpenSpec, feature checklist, measured≥95% core unit coverage,
  actual gameplay collision/input tests, visual inspection and focused manual list.
- [ ] Extended travel/turn/jump/turbo, slope matrix, expanded footprint, stairs/
  interlocks, sanctuary, atomic failures, costs, peaceful progression, seeded
  visitation/reload, restore supports, interrupted save recovery, bounded state,
  real packaged input. Automated success never implies human acceptance.
- [ ] Future only: heading hold/route automation; persistent pocket doors;
  greenhouse/apothecary/chart room; new biomes/families; deeper mines/populated
  towns/more graphs. Initial scope bounded physical rooms/manual helm/covered
  mines/abandoned hamlets.
