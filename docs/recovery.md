# Workspace recovery note

The initial working directory held a separate Godot project. It was left intact.
A new independent native checkout was created beneath it as wandering-native.

The Windows host disconnected during an uncommitted entrance-bell follow-up.
That local source directory contains an unfinished wc_outdoor_target edit and
must be reviewed before reuse. Do not discard local changes blindly.

The canonical completed native source is now the GitHub work branch. Lua/rule
changes were reconstructed from successful session edits and reverified by CI.
The unfinished bell edit was excluded. The local and GitHub commit metadata
also differ because publication used the connected GitHub API while shell
network access was unavailable.

On reconnection, preserve the old native directory, clone the work branch into
a fresh directory, and compare any desired local changes before applying them.
Do not reset or overwrite the existing Godot project.
