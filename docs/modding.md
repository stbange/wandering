# Editing gameplay

Edit content/home.lua beside the executable and relaunch. Tuning, object bounds,
recipe costs/rewards and crop durations are validated before use.
Items are ordered: timber, stone, fiber, sunroot seed, mint seed, plank, twine,
sunroot, mint. Recipe IDs must be unique; quantities must be non-negative integers.

Edit content/rules.lua to map home panel choices to commands. The script returns
a table with on_action(panel,index,slot). A result is a table such as
{command="craft", id=1}. Indices are zero-based. Available commands: craft, garden,
rest, save, select-item, deposit, withdraw. C checks authoritative constraints.

Rules have an instruction budget and no OS, filesystem or network libraries.
Treat mods as trusted local code. This is not a general hostile-code sandbox.

Rendering art is external OBJ meshes, PPM grain texture, text pixel font and GLSL
shaders. Code/scripts/shaders are MIT; original art is CC BY 4.0.
