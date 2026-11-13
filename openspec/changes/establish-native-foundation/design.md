# Decisions

Use a fixed 120Hz authoritative C simulation. Store the attached player in
castle-local coordinates; transform once for rendering. Use smooth terrain
queries for support. Preserve independent local view yaw/pitch across movement.

Use OpenGL3.3 / GLES3 shader/VBO rendering with SDL3 platform events and a Lua5.4
content loader. Keep art files and script tables outside the executable.
Test commands without SDL; test event mapping and real GL rendering separately.

Version and checksum saves; validate into temporary state before a controlled
restore. Return to parked on-foot state and clear held controls.

# Risks and boundaries

The detailed game is much larger than the foundation. Track future requirements
explicitly. Never represent a desktop build as WebXR support. Avoid importing
unlicensed franchise material. Do not claim whole-app coverage from core tests.
