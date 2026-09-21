---
name: recompile
description: Rebuilds this project's C++ game code (Source/SciFiTowerDefense/) via Unreal Engine's Live Coding build path and hot-patches it straight into the already-running Unreal Editor - no need to alt-tab to the editor and hit the Live Coding hotkey. Use this immediately after editing any .h/.cpp file under Source/SciFiTowerDefense/, whenever the user says "recompile", "rebuild", "build the C++", or "compile", and proactively before trying to set a newly-added UPROPERTY or call a newly-added UFUNCTION through the unreal-mcp tools - those won't exist in the running editor until this build succeeds. Do NOT use this for Blueprint-only edits (use BlueprintTools.compile_blueprint for those instead) or when the Unreal Editor isn't currently running.
---

# Recompile C++ (Live Coding)

## Why this exists

This project's C++ changes are normally picked up by pressing Ctrl+Alt+F11 in the
Unreal Editor, which triggers Unreal's Live Coding system: a targeted rebuild that
patches new code into the *already-running* editor process instead of restarting it.
This skill runs the exact same build UBT (UnrealBuildTool) would run for that
hotkey, so C++ changes can be compiled and picked up without leaving the terminal
or asking the user to switch windows.

This only works while the editor is open — Live Coding patches a running process.
If the editor isn't running, run a normal build instead (or just start the editor).

## Run the build

Use the PowerShell tool with a generous timeout (this can take anywhere from ~10
seconds for a one-line change to a couple of minutes for something touching many
files) — pass `timeout: 300000` (5 minutes) rather than relying on the 2-minute
default:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" -Target="SciFiTowerDefenseEditor Win64 Development" -Project="C:\Users\MichaelMoncrief\Documents\Unreal Projects\SciFiTowerDefense\SciFiTowerDefense.uproject" -LiveCoding -LiveCodingModules="C:\Program Files\Epic Games\UE_5.8\Engine\Intermediate\LiveCodingModules.json" -LiveCodingManifest="C:\Program Files\Epic Games\UE_5.8\Engine\Intermediate\LiveCoding.json" -WaitMutex -LiveCodingLimit=100
Write-Output "EXIT CODE: $LASTEXITCODE"
```

These paths and the target name (`SciFiTowerDefenseEditor`) are fixed for this
machine and this project — they don't need to be rediscovered each time.

`-WaitMutex` makes the build wait its turn instead of failing outright if the
editor is already mid-compile (e.g. the user pressed the hotkey at the same time).

## Reading the result

- **Success**: exit code `0`, and the tail of the output includes a line like
  `Result: Succeeded` along with a "Total execution time" line. New C++
  UPROPERTYs/UFUNCTIONs are now live — you can immediately query or set them via
  the project's unreal-mcp tools (e.g. `ObjectTools.list_properties` should now
  show them).
- **Failure**: non-zero exit code, and the output contains compiler error lines
  (typically `error C####:` or `error :` with a file path and line number). Read
  those lines to find the actual problem — don't just retry the build. Fix the
  reported error in the source file, then run the build again.
- If the output mentions the editor process wasn't found or Live Coding isn't
  enabled, the editor likely isn't running (or Live Coding is off in its
  settings) — tell the user rather than guessing further.

## After a successful build

Anything that depended on a property or function existing (e.g. a previous
`set_properties` call that failed because a UPROPERTY didn't exist yet) can now be
retried — the compiled class has been hot-reloaded into the live editor session.
