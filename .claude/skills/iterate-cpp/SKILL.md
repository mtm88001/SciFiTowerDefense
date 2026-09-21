---
name: iterate-cpp
description: Runs the complete C++ change/verify loop for SciFiTowerDefense - edit code, recompile, confirm the recompile actually took effect (not just that the build reported success), fall back to a full close-editor/rebuild/open-editor cycle if Live Coding silently failed to apply it, then play-test the actual behavior and stop PIE again. Use this as the default way to make and verify any nontrivial C++ change in this project, especially anything touching a function that's already been Live-Coding-patched earlier in the session (a case proven unreliable here). Also covers when it's fine to leave temporary debug logging in place mid-loop rather than cleaning up immediately.
---

# The full C++ change loop

This ties together four things learned the hard way in this project: Live Coding can
report success while silently failing to re-patch an already-patched function; a
Blueprint-driven construction script (like a spawned mesh chain) needs an explicit
`compile_blueprint` to actually re-run after a C++ change, not just a recompile;
`StartPIE` can cry wolf; and closing/reopening the editor is sometimes genuinely
necessary, not a failure to route around.

## 1. Make the code change

Just edit the .h/.cpp as normal.

## 2. Recompile - try Live Coding first

Use the `recompile` skill (or its build command directly). It's fast and doesn't
require touching the editor. Most changes - especially the *first* Live Coding patch to
a given function since the editor last (re)started - apply correctly this way.

## 3. Verify the recompile actually took effect - don't just trust "succeeded"

This step is tempting to skip and shouldn't be. "Result: Succeeded" and "Live coding
succeeded" in the logs only mean UnrealBuildTool compiled the code and the editor
acknowledged a patch attempt - not that the specific function changed is what's actually
running. Pick whichever of these fits the change:

- **New UPROPERTY/UFUNCTION**: check `ObjectTools.list_properties` on the relevant
  class/CDO for the new member. Reflection changes have been reliable via Live Coding
  even when function-body patches to the same file weren't.
- **Changed logic inside an existing function**: add a temporary, unmistakable
  `UE_LOG(LogTemp, Warning, TEXT("..."))` line with an actual runtime value from the new
  code path, recompile, trigger the code path (e.g. `compile_blueprint` if it's a
  construction script, or a `playtest` run if it's gameplay logic), then check
  `EditorToolset.LogsToolset.GetLogEntries` for that exact marker. If it never appears,
  the patch didn't really apply, no matter what the build reported.
- It's fine to leave this logging in the file across iterations while still chasing
  something down - don't stop to clean it up mid-loop. Pull it once the behavior is
  confirmed correct and the fix is wrapped up (a good last step, not a prerequisite for
  testing).

## 4. If a placed actor's construction script needs to re-run

A C++/DLL change alone doesn't retroactively re-execute code that already ran (e.g. a
spline mesh chain built in `OnConstruction`). Call
`editor_toolset.toolsets.blueprint.BlueprintTools.compile_blueprint` on the relevant
Blueprint to force it to reconstruct, then re-verify.

Watch out for the trap this hit once already: if a construction script tracks
dynamically-created components in a `Transient` array for cleanup, that array does
*not* survive a level save/reload, while `AddInstanceComponent`-registered components
*do*. Cleanup logic needs to sweep the actor's actual attached children, not just the
tracking array, or a level reload leaves stale components in place and a fresh
reconstruction piles new ones on top without removing them.

## 5. If verification shows the patch didn't apply

This means Live Coding silently failed to re-patch the function - which happens
reliably for a function that's already been hot-patched earlier in the same session.
Don't keep retrying Live Coding; it won't newly succeed against the same stale patch
state. Escalate straight to a real rebuild:

1. Use the `close-editor` skill.
2. Run the *normal* (non-`-LiveCoding`) build: the `recompile` skill's build command
   with the `-LiveCoding`/`-LiveCodingModules`/`-LiveCodingManifest`/`-LiveCodingLimit`
   flags removed (keep `-WaitMutex`). Confirm the output includes an actual
   `Link [x64] UnrealEditor-SciFiTowerDefense.dll` step - that's the signal this is a
   real relink, not another patch attempt.
3. Use the `open-editor` skill.
4. Re-run step 3/4's verification against the fresh process before trusting it this time.

## 6. Play-test the real behavior

Use the `playtest` skill. A clean recompile is necessary but not sufficient - actually
exercise the change in PIE and check real state, not just that the code compiled.

## 7. Report clearly what was verified

State what was actually confirmed (a log line fired, a property matched, PIE behavior
was correct) rather than "recompiled successfully" - that phrase alone has been true
several times in this project while the actual fix wasn't live yet.
