# Product contract

The full user brief is represented by the OpenSpec journey specification and
the feature matrix in verification.md. Non-negotiable invariants: first person,
an always-safe home, peaceful essential progression, external editable content,
atomic economy, no mandatory fuel/food/maintenance, and persistent stable worlds.

Default input: WASD move / helm; mouse or arrows look; Space jump; Shift sprint /
5x helm turbo; E interact / release helm; I inventory; B build; R rotate; F
furniture; X/LMB melee; Q ward; K techniques; V Second Wind; C Power Strike;
Escape close/pause; F1/Tab help; F5 save; F9 restore. Touch uses on-screen move,
look and action controls. Menus retain the camera and pause active simulation.

Initial home 12x16m, eye 1.62m, six legs, level cabin; speed 5/25m/s, turn
0.38rad/s. Rooms: 12x16m halls, 6x6m annexes; upward-first; cap 12. Ordinary
room cost 6 planks + 2 twine + 4 stone. No global character level gates regions.

Items, recipes, crop timing, trader prices and progression definitions are in
assets/scripts/content.lua. World and save versions must be changed when their
corresponding deterministic contracts become incompatible.
