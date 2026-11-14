# GitHub Pages

The standard browser game and WebXR build have separate entry pages:

- Standard game: https://stbange.github.io/wandering/
- WebXR edition: https://stbange.github.io/wandering/webxr/

## One-time repository setup

An owner must open [Settings → Pages](https://github.com/stbange/wandering/settings/pages)
and select **GitHub Actions** under **Build and deployment → Source**.
The connected GitHub App can push the workflow but its attempt to enable Pages
returned HTTP 403, `Resource not accessible by integration`. This setup is an
account-permission requirement; it cannot be granted by the workflow's token.
No extra token or secret is needed for subsequent deployments.

## Automatic publication

Pushes to `main` build `WC_WEBXR=OFF` and `WC_WEBXR=ON` independently. The staging
script publishes them as `index.html` and `webxr/index.html`, each with its own
WASM, JavaScript, editable assets and license notices. The browser tests run
below `/wandering/` to catch project-prefix path mistakes before publication.

After native builds, the coverage gate and both browser suites pass, the Pages
job publishes the artifact. Pull requests never publish. If verification ran
before Pages was enabled, re-run the failed Pages job after enabling it; the
uploaded artifact is retained by GitHub Actions.

The standard page never probes XR hardware or exports the XR bridge. The WebXR
page has a dedicated headset entry button and shows availability feedback on
devices without immersive VR support. API integration tests do not substitute
for physical headset acceptance.

## Local reproduction

```sh
emcmake cmake -S . -B build-web-standard -DWC_WEBXR=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-web-standard --parallel 4
emcmake cmake -S . -B build-web -DWC_WEBXR=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --parallel 4
python3 tools/stage_pages.py --web build-web-standard --webxr build-web --output build-pages/wandering
python3 -m http.server 8091 --directory build-pages
```

In another terminal, run `node tests/pages.mjs`. The full game journey can run
against the staged standard page by setting
`WC_URL=http://127.0.0.1:8091/wandering/?seed=42`. Run `tests/input.mjs` with
`WC_URL=http://127.0.0.1:8091/wandering/webxr/?seed=42` for XR/input integration.
