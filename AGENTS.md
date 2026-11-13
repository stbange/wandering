# Repository workflow

- Work in small runnable increments. After each completed implementation step, commit and push to the working branch.
- Never mark human acceptance complete from an automated test or a commit.
- C owns authoritative simulation, validation, collision and persistence. Lua owns editable content and scenario scripts. Rendering and input adapters do not mutate state directly.
- Keep code under MIT and original art under CC BY 4.0. Keep dependency licenses intact.
- Validate OpenSpec changes and run the relevant tests before committing. Record actual coverage, platform evidence and remaining gaps.
- Do not replace failures with skipped tests, weaken assertions to match bugs, or represent unrun checks as passes.
