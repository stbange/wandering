# Separate GitHub Pages editions

Publish the standard browser game at the repository's Pages root and an
independent WebXR build at `webxr/`. Completed main-branch packages deploy
automatically after native, coverage and browser verification pass.

The standard edition omits the XR bridge and immersive-session entry point.
The WebXR page identifies itself and offers an explicit headset entry button,
with readable availability feedback when no compatible headset is present.
Both pages retain keyboard/mouse/touch first-person preview and editable assets.
