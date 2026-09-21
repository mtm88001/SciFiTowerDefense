---
name: open-editor
description: Relaunches the Unreal Editor for this project (SciFiTowerDefense) and waits until the level has actually finished loading before doing anything else - not just until the process starts. Use this right after a normal (non-Live-Coding) C++ build that required the editor to be closed first (see the close-editor skill), or whenever the user says to reopen/relaunch the editor.
---

# Reopen the Unreal Editor

## Steps

1. **Launch it, detached** (don't block waiting for it to exit - it runs indefinitely):
   ```powershell
   Start-Process -FilePath "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList '"C:\Users\MichaelMoncrief\Documents\Unreal Projects\SciFiTowerDefense\SciFiTowerDefense.uproject"'
   ```

2. **Wait for the level to finish loading - don't just wait a fixed number of seconds.**
   Startup time varies a lot (shader compilation, asset loading), and unreal-mcp tool
   calls will fail or hang if made before the editor is ready. Poll the project's log
   file for the line Unreal prints once map loading completes, using a *backgrounded*
   Bash wait (not a blocking sleep, and not a tight foreground loop):
   ```bash
   until grep -q "MapCheck: Map check complete" "/c/Users/MichaelMoncrief/Documents/Unreal Projects/SciFiTowerDefense/Saved/Logs/SciFiTowerDefense.log" 2>/dev/null; do sleep 2; done; echo "EDITOR READY"
   ```
   Run this with `run_in_background: true` so you get a single notification when it's
   ready instead of blocking or polling yourself. Unreal starts a *fresh* log file on
   each launch (the previous one is renamed to a `-backup-...` file), so this grep is
   always reading the current session, never stale data from before the restart.

3. **Verify the connection is actually back**, e.g. with a lightweight call like
   `EditorToolset.LogsToolset.GetLogEntries`. A "still connecting" system notice for the
   unreal-mcp server can appear right after relaunch even when a direct tool call
   already succeeds - try the call rather than waiting further on the notice alone.

4. **If you were mid-fix** (the usual reason to reopen), remember that **Live Coding
   patches don't survive a restart** - the editor loads whatever was last written to
   disk by a real build. If a fix was ever only applied via a `-LiveCoding` build,
   re-confirm it's still present (re-check the property/behavior it changed) rather
   than assuming the restart preserved it.
