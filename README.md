# Wandering Castle

A first-person exploration game about a safe home on six legs. Prepare at home,
steer the Hearthstrider through the landscape, park, explore, and bring useful
discoveries back to a growing home.

This is the **C17 / SDL3 / OpenGL / Lua** implementation in
[stbange/wandering](https://github.com/stbange/wandering). It is independent of
the earlier Godot prototype. The detailed Hearthstrider requirements govern
this implementation; the sentient multiverse-travelling van premise is recorded
as an unresolved narrative direction, not an implemented opening.

[Download Windows build](https://github.com/stbange/wandering/actions/runs/43420433941/artifacts/14034036959) · [Verification](https://github.com/stbange/wandering/actions/runs/43420433941)

**Playable Windows foundation:** walk and jump around the furnished moving
home, steer the physical helm, deploy stairs, gather nearby supplies, garden,
craft a satchel, and save/restore. Run `./tools/build.ps1 -Mode Run` after the
[build setup](docs/build.md). The full game is not complete.

The current implementation is on the `work` branch. Extract a Windows build and
launch `wandering.exe`, keeping SDL3.dll, assets/ and content/ beside it.
Lua extension details are in [modding](docs/modding.md).

Development is incremental. See [feature status](docs/feature-status.md), [the plan](docs/plan.md),
[requirements](docs/requirements.md), [verification](docs/verification.md), and
[OpenSpec](openspec). A checkmark means implemented or verified only in its
named column. **Human acceptance is pending.**

Code, Lua scripts and documentation: [MIT](LICENSE).
Original visual/audio assets: [CC BY 4.0](assets/LICENSE.md).
Third-party libraries retain their own licenses.
