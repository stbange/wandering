# Pages acceptance

- Opening the project root starts the standard WebGL2 game.
- Opening `webxr/` starts the independently built WebXR edition.
- Both URLs work below a GitHub project prefix, without missing WASM, Lua,
  shader, texture, font or audio requests. Neither page redirects to an artifact.
- Each page links to the other using a project-relative URL.
- The standard page does not probe XR hardware or expose the XR bridge.
- The immersive page shows a supported headset entry action or useful
  availability feedback. Headset acceptance remains separate from API tests.
- A failed build or test does not deploy. Pull requests never deploy.
- Successful pushes to main publish both editions as one Pages artifact.
