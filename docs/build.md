# Build and run

## Download and play

[Windows build](https://github.com/stbange/wandering/actions/runs/43420433941/artifacts/14034036959): extract the ZIP and launch
wandering.exe. Keep SDL3.dll, assets/ and content/ beside it. Use the work branch
for source. Other build artifacts and exact verification are linked in verification.md.

## Windows: portable toolchain used for verification

From a PowerShell prompt in this repository:

```powershell
./tools/bootstrap-windows.ps1
./tools/build.ps1 -Mode Test
./tools/build.ps1 -Mode Run
./tools/build.ps1 -Mode Coverage
./tools/build.ps1 -Mode Package
```

The bootstrap downloads pinned portable tools into `.deps/`, verifies SHA256
and extracts them. It does not change system installations or global PATH.
The game is `build/wandering.exe`; packages are in `dist/`. Keep `SDL3.dll`,
`assets/` and `content/` beside the executable. Launch from any working directory.
Edit `content/home.lua` before launching to change tuning, furnishings, recipes
and crop durations; `content/rules.lua` controls home activity dispatch. See modding.md.

## Generic CMake (Windows, Linux, macOS)

Requires CMake >=3.24, a C17 compiler and OpenGL3.3. SDL3 can be supplied with
`SDL3_DIR`; otherwise CMake downloads the pinned SDL3 source. Lua5.4.8 is fetched
and built statically, or supply `WC_LUA_SOURCE` to an extracted source tree.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix dist/wandering
```

On Linux, install development packages for OpenGL and SDL's window backends
(e.g. `libgl1-mesa-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev
libxi-dev libxfixes-dev libxss-dev libxtst-dev libwayland-dev libxkbcommon-dev`).
Use `xvfb-run -a ctest --test-dir build --output-on-failure` for a virtual display.

Core-only tests need no window system:

```sh
cmake -S . -B build-core -DWC_BUILD_GAME=OFF -DWC_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
python tools/coverage.py build-core
```

Coverage requires GCC/gcov. The gate is >=95% authored authoritative C **line**
coverage. Branch coverage is reported too. SDL/OpenGL adapter coverage is not
included; the separate executable game test exercises actual events/rendering.

## Emscripten build path (CI compilation verified; runtime pending)

Activate an Emscripten SDK, then:

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --parallel
python -m http.server 8000 --directory build-web
```

Open `http://localhost:8000/wandering.html`. The `.data` package contains external
assets/scripts and is separate from wasm. SDL touch events select a left movement
drag, right look drag and interaction/jump/pause buttons. Browser persistence,
mobile usability and WebXR remain unverified/incomplete; do not advertise them
as released platforms. The desktop game is the verified deliverable.

## Controls in this increment

WASD move. Mouse or arrows look. Space jumps. Shift sprints; at helm it enables
5x turbo. Aim at the helm and E to control it, E again to brake/release. Close
the gate before steering. Aim at the brass lever beside the rear gate and E
to deploy stairs. Aim at furniture and E for paused activity panels. I opens
the backpack. F1/Tab toggle help. Escape pauses/closes a panel. F5 saves and
F9 restores. Mouse buttons or number keys select panel actions.

The pause menu offers recovery of the previous save and confirmed New Journey.
Native saves use SDL's per-user preference directory. Starting a new journey
does not overwrite an existing save until Save is chosen. Game tests use their
own `game-test.sav` in the test directory, not the user's journey.
