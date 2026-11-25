# Trajectory implementation research

## Reference implementation

The public Eclipse Menu source contains a `ShowTrajectory.cpp` implementation:

- Reference repository: https://github.com/Uproxide/EclipseMenu
- Reference file: https://github.com/Uproxide/EclipseMenu/blob/main/src/hacks/Player/ShowTrajectory.cpp
- A maintained fork with the same feature structure was also inspected: https://github.com/Srivanthv/eclipsemenu/blob/main/src/hacks/Player/ShowTrajectory.cpp

This document records architectural findings only. GeoDan must implement its own code; do not copy the reference file wholesale.

## What Eclipse does

1. Creates two hidden `PlayerObject` instances and adds them to the active play layer's object layer.
2. On each simulation refresh, copies the live player's attributes into each simulation player.
3. Tracks whether the real player is currently holding jump.
4. Simulates one branch with jump held and one branch with jump released.
5. Uses the active play layer's collision checker against the real level objects.
6. Advances each simulation player using `PlayerObject::update(dt)` and (in one revision) `updateRotation(dt)`.
7. Draws the segment between successive simulated positions.
8. In dual mode, performs the same process for the second player.
9. Hooks several game functions to suppress simulation deaths and selected side effects.

The simulation frame delta is derived from the live update delta and the level time-warp value. Eclipse uses a configurable iteration count (300 in one revision).

## Important safety findings

Eclipse demonstrates that using real Geometry Dash player physics is practical, but it does not prove the simulation is perfectly isolated. Its implementation uses a global simulation flag and selectively suppresses side effects.

The inspected code explicitly guards or filters several operations:

- Simulation player deaths are intercepted in `PlayLayer::destroyPlayer`.
- End animations and gravity-flip handling are suppressed while simulating.
- Trigger object execution is suppressed.
- Shine effects, hard-streak updates, spider-dash effects, jump counters, ring jumps, and ring-touch handling are suppressed.
- `canBeActivatedByPlayer` returns false during simulation.
- The collision candidate list is filtered to solid, hazard, animated-hazard, and slope objects so triggers are not activated early.
- Per-player collision logs are cleared between simulation iterations.

These are useful safeguards, not a complete proof that every side effect is blocked. Before adopting the approach, GeoDan must audit the target 2.2081 bindings and relevant call paths for additional shared state, audio, particles, camera changes, checkpoint behavior, object mutation, and other callbacks.

## Proposed GeoDan architecture

### A. Simulation lifecycle

- Own two hidden simulation players per active `PlayLayer`, with explicit creation, attachment, cleanup, and recreation rules.
- Never allow simulation objects to be confused with `m_player1` or `m_player2`.
- Keep a scoped simulation context (RAII guard) rather than relying on a manually toggled flag that can remain set after an early return.
- Ensure cleanup on level exit, restart, editor transition, and failed initialization.
- Reinitialize each branch from the live player's attributes before every prediction refresh.

### B. Two branches

- Branch A: preserve the live jump-held state, then continue holding.
- Branch B: preserve the live jump-held state, then release.
- If the player is already holding jump, preserve that initial input state correctly; do not inject an artificial new press.
- Handle player 1 and player 2 independently in dual mode.
- Derive simulation delta from the real frame delta and current time warp; validate pause, slow motion, and frame-step behavior.

### C. Collision and trigger policy

- Use the real level collision system for solid, hazard, animated-hazard, and slope collision.
- Exclude trigger/activation objects from simulation collision candidate lists.
- Suppress activation and other irreversible effects during prediction.
- Preserve and restore any collision candidate vectors if the original function expects in-place mutation.
- Do not assume filtering the object list alone makes the call side-effect-free.
- If a gameplay interaction cannot be safely simulated (rings, pads, portals, teleporters, force objects, gravity/speed/size changes, dual switches, moving objects, etc.), either implement a safe simulated equivalent or terminate/mark the branch as uncertain. Do not activate the real object.

### D. Rendering

- Draw two independently coloured paths with a configurable iteration/horizon limit.
- Keep drawing separate from simulation.
- Clear paths when disabled, when the player is invalid/dead, and during lifecycle transitions.
- Render optional simulated hitboxes only if they can be drawn without changing simulation state.
- Avoid a process-global singleton draw node; tie ownership to the active play layer.

### E. Performance

- Avoid allocating large temporary containers every frame where possible.
- Reuse simulation players and draw buffers.
- Make iteration count configurable with safe bounds.
- Measure the update cost once in-game; do not claim performance is acceptable before profiling.

## Required implementation review checklist

- [ ] Confirm all referenced 2.2081 members and method signatures from Geode bindings.
- [ ] Audit every hook that can run during `checkCollisions`, `PlayerObject::update`, and `updateRotation`.
- [ ] Prove simulation death cannot kill or end the real run.
- [ ] Prove simulation cannot activate or mutate real triggers/objects.
- [ ] Prove collision candidate filtering restores original state on all paths.
- [ ] Prove both players' input state is handled independently in dual mode.
- [ ] Prove simulation players and draw nodes are cleaned up on every lifecycle path.
- [ ] Add isolated tests for lifecycle/state logic and any pure helper functions.
- [ ] Build against Geode 5.11.2 / Geometry Dash 2.2081.
- [ ] In-game test ordinary cube, ship, wave, ball, UFO, robot, spider, swing, slopes, portals, triggers, dual, speed/timewarp, and death/restart/quit transitions.
- [ ] Profile frame cost at several prediction iteration counts.

## Current implementation status (2026-11-25)

The isolated Codex candidate now has a first game-facing implementation in:

- `tools/src/trajectory.cpp` / `tools/src/trajectory.hpp` — owns two hidden simulation players, copies the live player's state, simulates independent hold/release branches through the game's player update and collision routines, and draws the predicted segments.
- `tools/src/overlay.cpp` — updates the trajectory after the live PlayLayer update and clears/releases it on reset/quit.
- `core/trajectory_model.hpp` — bounded path storage, terminal reasons, and scoped simulation context.

The implementation suppresses known side effects (simulation deaths, end animation, gravity flips, trigger activation, ring handling, selected player effects, game events, particles, and hard-streak updates). Collision candidates are filtered to ordinary solids/hazards/slopes and restored after the collision call. Unsupported candidates and known moving objects terminate that branch as uncertain.

**This is an experimental candidate, not a verified or promoted feature.** The Geode build and in-game tests have not been run. The binding declarations were checked against Geode's 2.2081 `GeometryDash.bro`, but bindings do not expose every internal side effect. In particular, the simulation still needs a Geode build and in-game safety audit for hidden shared-state changes, audio, moving/rotating geometry, all gamemodes, timewarp, dual mode, and lifecycle transitions. Do not claim complete side-effect isolation until those tests pass.
