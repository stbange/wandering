# Why

Home activities need editable Lua behavior in addition to tuning and furnishing data.

# What Changes

- Move recipes and crop tuning to external Lua data.
- Route home panels through an editable Lua command script.
- Retain authoritative atomic validation in C.
- Package content/licenses and configure platform CI.

# Capabilities

## New Capabilities
- `scripted-home`: editable, validated home data and command dispatch.

# Impact

New external rules file, startup validation, test cases and platform build jobs.
The current save layout is unchanged. Full game and platform acceptance remain pending.
