# Architecture

The C simulation is a deterministic fixed-step state machine. All commands
validate before modifying live state. Stable IDs identify content, storage,
rooms, rewards and world mutations. A command result is the source for feedback;
the HUD reads the same authoritative state as tests and saving.

Coordinates use a double precision logical castle position and small local
player coordinates. Indoor player movement and collision happen in castle
space. Rendering transforms once into world space. Platform motion must never
be applied both to the local player and camera. Mouse yaw/pitch are incremental;
mouse deltas are not multiplied by frame time. Pitch clamps each event and drops
overshoot. Arrow rotation is time-based and never changes movement axes.

SDL supplies events, windows, timing and audio. The input adapter maps events
into gameplay, helm, construction or menu commands. Losing focus pauses and
clears held input. Menus release the pointer. Restoring resets to on-foot parked
state after support has been reconstructed. No renderer may change gameplay.

The OpenGL renderer uses shader programs, vertex buffers and external meshes;
the same geometry path targets desktop OpenGL 3.3 and WebGL2 / GLES3. No legacy
fixed-function GL. CPU terrain heights also drive collision. Distant visuals
are explicitly non-colliding. Generated terrain must not be treated as loaded
collision until attached on the main thread.

Lua tables define tuning, furnishings, recipes and scenarios. They are loaded
from `content/` beside the binary, validated by C and editable without compiling.
Lua cannot bypass authoritative inventory or placement validation. Save files
are data, never executable Lua. Assets live in `assets/` beside the executable.
Web builds fetch a separate filesystem data package; assets are not compiled
into wasm and can be repackaged independently.

Persistence stages and validates a full candidate state. A version, checksum,
range checks and atomic publication protect live state and the last generation.
Native saves live in SDL's user preference directory, not beside a read-only
installation. Web persistence uses IDBFS and explicit sync, with failures exposed.

Future seams: steering controller (manual / heading hold / route), world provider
(terrain / pocket room), room capabilities (garden / workshop / chart room),
expedition layout family and XR presentation/input adapters. These are not claims
that future features exist.
