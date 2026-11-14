# Implementation plan

1. Build a renderer-independent C core and test command failures before UI.
2. Add Lua loading, SDL3 first-person loop and low-poly GL presentation.
3. Exercise movement, boarding and the peaceful home expansion loop.
4. Add persistence fault tests, coverage gate and browser-driven game tests.
5. Package native/web assets beside the program and record platform evidence.

Specs change before behavior changes. Each increment must compile and retain a
runnable state. Future changes should add OpenSpec change proposals, tests and
verification evidence, then merge stable behavior into openspec/specs.
