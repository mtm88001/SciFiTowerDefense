---
name: playtest
description: Starts a Play-In-Editor (PIE) session for SciFiTowerDefense to test a gameplay change, inspects the running actors/state to verify the change actually works, and - critically - stops PIE again afterward. Use this whenever the user asks to test, verify, or check a change in-game, or after any gameplay-affecting C++/Blueprint edit that needs real verification rather than just "the build succeeded." Do NOT leave PIE running when done: a running PIE session blocks normal (non-Live-Coding) rebuilds and makes it easy to accidentally query stale UEDPIE_0_-prefixed actor paths instead of the editor world's.
---

# Play-test a change

## Start PIE

```
EditorToolset.EditorAppToolset.StartPIE
  options: { bSimulate: false, playMode: "PlayMode_InViewPort", warmupSeconds: 4-5 }
```

`StartPIE` has occasionally reported "PIE ended before warmup completed" even though PIE
is actually running fine underneath - don't trust that error message on its own.
Immediately confirm the real state with `EditorToolset.EditorAppToolset.IsPIERunning`.

## Inspect what actually happened

Prefer checking real state over screenshots where possible - it's more precise and
doesn't depend on finding a usable camera angle:

- Find the actors you care about: `editor_toolset.toolsets.scene.SceneTools.find_actors`
  with the relevant `actor_type`. Note PIE actor paths live in a *different* world,
  prefixed `UEDPIE_0_` (e.g. `.../Maps/UEDPIE_0_TD_Prototype.TD_Prototype:PersistentLevel...`),
  not the editor world's own paths.
- Read live properties/transforms: `ObjectTools.get_properties` /
  `ActorTools.get_actor_transform`.
- For anything geometry/collision-related, `SceneTools.trace_world` (start/end points,
  returns hit distance) is a fast, precise way to confirm a surface height or that
  collision exists at a spot, without needing a screenshot at all.
- Check `EditorToolset.LogsToolset.GetLogEntries` for errors/warnings, or for any
  diagnostic `UE_LOG` lines deliberately left in for this test.
- A screenshot (`EditorToolset.EditorAppToolset.CaptureViewport`) is worth it for things
  that are genuinely about appearance, but `captureTransform` has been unreliable in
  this project for oblique angles - top-down (`pitch: -90, yaw: 90`) is the one that's
  actually worked reliably. Don't burn a lot of time fighting the camera; the state
  checks above are usually enough to confirm correctness. A single screenshot also
  can't show a *temporal* issue like flickering - for that, the user's own live view of
  the editor is the more reliable source, so ask them to look rather than trying to
  prove it from a static capture.

## Stop PIE - do not skip this

```
EditorToolset.EditorAppToolset.StopPIE
```

Do this as soon as you've gathered what you need. Leaving PIE running is the easiest way
to make the next step in a dev loop (a rebuild, or another test) confusing or impossible.
