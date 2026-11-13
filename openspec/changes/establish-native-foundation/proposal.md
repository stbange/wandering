# Why

The requested repository is empty. A native C/SDL3/OpenGL/Lua implementation
needs a runnable, measured foundation independent of the Godot prototype.

# What Changes

- Establish licenses, external content layout, build/test/packaging and specs.
- Implement first-person motion and collision in the moving castle's local frame.
- Add physical helm interaction, incremental look, pause/focus contexts and saves.
- Verify via core unit coverage and actual SDL event/rendered gameplay tests.

# Capabilities

## New Capabilities
- `moving-home`: first-person motion, support, helm and input lifecycle.
- `native-foundation`: C/Lua boundary, assets, validation and packaging.

# Impact

New repository only. No existing Godot files or saves are migrated or modified.
The full contract remains in docs/requirements.md with unchecked later features.
