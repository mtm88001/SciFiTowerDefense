---
name: close-editor
description: Safely closes the running Unreal Editor for this project (SciFiTowerDefense) by saving all dirty assets via the unreal-mcp tools first, then stopping the UnrealEditor.exe process. Use this whenever a normal (non-Live-Coding) C++ build is needed - Windows won't let a normal build overwrite UnrealEditor-SciFiTowerDefense.dll while the editor still has it loaded - or whenever the user asks to close/restart/relaunch the editor. Do NOT use this just to stop a Play-In-Editor session (use the playtest skill's StopPIE step for that); this closes the whole editor application.
---

# Close the Unreal Editor

## Why this exists

A normal (non-Live-Coding) build links a fresh copy of `UnrealEditor-SciFiTowerDefense.dll`
on disk, which Windows refuses to do while the running editor still has that DLL loaded.
Live Coding avoids this by patching the running process in memory instead - but Live
Coding has proven unreliable in this project for re-patching a function that was already
hot-patched earlier in the same editor session (confirmed by adding a `UE_LOG` marker
inside a patched function and watching it never fire, despite every build/Live-Coding
signal reporting success). When that happens, the only reliable fix is a real rebuild,
which means closing the editor first.

## Steps

1. **Save first, if the editor is reachable.** If the unreal-mcp connection is currently
   working, call `editor_toolset.toolsets.asset.AssetTools.save_assets` with an empty
   `asset_paths` list (saves everything dirty) before closing anything. Skip this only
   if the editor/MCP connection is already unresponsive.

2. **Find the process:**
   ```powershell
   Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue | Select-Object Id, ProcessName, StartTime
   ```
   There should normally be exactly one. If there are several, they may be from
   different projects - only close the one that matters, or ask the user which.

3. **Stop it:**
   ```powershell
   Stop-Process -Id <the Id from step 2> -Confirm:$false
   ```

4. **Confirm it's actually gone** before doing anything that assumes the editor is
   closed (like a normal build):
   ```powershell
   $procs = Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue
   if ($procs) { Write-Output "STILL RUNNING: $($procs.Id)" } else { Write-Output "NOT RUNNING" }
   ```
   `Get-Process` returning nothing can make this tool report a nonzero exit code even
   though nothing actually went wrong - trust this explicit printed check, not the raw
   exit code.
