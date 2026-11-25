# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read [`README.md`](./README.md) and [`claude-check-list.md`](./claude-check-list.md) as well as this file.

## Current status

- **Status:** Active — Eclipse reference architecture reviewed; implementation design updated.
- **Active task:** Implement a safe, GeoDan-owned trajectory simulator using verified Geometry Dash 2.2081 behavior.
- **Current stage:** Reference research and safety design.
- **Baseline at checklist creation:** `main` — `8ce48311b51acae5bff35a1e117847b77df70f13` (Claude checklist creation commit; verify current HEAD before relying on it).

## Project constraints

- Do not modify the archived `GeoDan-deprecated` repository.
- The old trajectory implementation is known to be broken. Do not consult, copy, port, or reuse it as a reference.
- Do not claim tests, builds, or in-game verification that were not actually performed.
- Read both agent checklists before project work and commit reviews.
- Update this file with concrete progress and a precise handoff before stopping.

## Active work

- **Status:** Active — isolated candidate, not approved for promotion.
- **Active task:** Rebuild the trajectory tool from verified Geometry Dash behavior and current Geode bindings; do not consult or reuse the old trajectory implementation.
- **Workspace:** `~/workflow-codex` (remote branch prepared; local checkout not yet verified or created).
- **Branch:** `workflow-codex/trajectory-rebuild`
- **Starting HEAD:** `6a2d5a1ffd2905911ac4f26c8a3829bcfb4b9bfd` (the current `main` head when this branch was created).
- **Current stage:** Eclipse reference research and architecture design. No retained trajectory implementation source is currently on this branch.
- **Promotion state:** Experimental; not approved for `~/main-workflow`.
- **Main workflow changed:** No.

## Decisions and findings

- **User decision:** Keep Codex and Claude implementations isolated in `~/workflow-codex` and `~/workflow-claude`; only user-selected changes may be promoted to `~/main-workflow`.
- **Verified repository finding:** `tools/mod.json` already has a disabled-by-default `trajectory` setting.
- **Verified repository finding:** Current gameplay overlay is `tools/src/overlay.cpp`; trajectory setting already exists in `tools/mod.json`.
- **User-approved reference:** Eclipse Menu's public `ShowTrajectory.cpp` may be studied as a technical reference. GeoDan must own its implementation; no wholesale copying.
- **Reference finding:** Eclipse uses two hidden `PlayerObject` instances, copies live player attributes, calls the real `checkCollisions` and `PlayerObject::update` routines, and filters/suppresses selected side effects through hooks.
- **Safety finding:** Eclipse's global simulation flag and selected hook guards are not proof of complete isolation. GeoDan must audit 2.2081 call paths for shared state, triggers, effects, audio/particles, camera, checkpoints, object mutation, and lifecycle cleanup.
- **Superseded prototype:** A simplified scalar-physics prototype was briefly added and then removed after inspecting Eclipse. It was not tested and must not be restored; using the game's actual player update/collision behavior is the preferred investigation path.
- **Reference notes:** `docs/trajectory-eclipse-reference.md` records the architecture, safety risks, and implementation checklist.
- **Constraint:** The old broken trajectory implementation remains explicitly forbidden as a reference.

## Test and build record

| Date | Command / test | Result | Notes |
|---|---|---|---|
| 2026-11-25 | GitHub source inspection and reference analysis | Completed | Inspected Eclipse `ShowTrajectory.cpp` and current Geode 2.2081 `PlayerObject.cpp` bindings. No build or tests run. |

## Files changed

- `docs/trajectory-eclipse-reference.md` — records Eclipse architecture, safety risks, proposed GeoDan design, and verification checklist.
- `checks/codex-check-list.md` — updated progress and handoff.
- A provisional scalar model, test, and overlay were added during exploration and then deleted; none remain in the branch.

## Handoff / next action

- **Last updated:** 2026-11-25.
- **Completed:** Inspected Eclipse's trajectory source and current 2.2081 Geode `PlayerObject.cpp`; documented the approach and its isolation risks. Removed the inadequate scalar-physics prototype.
- **Remaining:** Audit the exact 2.2081 declarations and collision/update call paths; inspect GeoDan build integration; design scoped simulation guards and lifecycle ownership; implement simulation players, collision filtering, rendering, tests, and settings integration.
- **Exact stopping point:** Branch contains the research document; no trajectory implementation remains. Local workspace path and checkout state are unknown.
- **Next action:** Continue static source/API audit and build a hook/side-effect matrix before writing the real simulator. Then implement against the current `tools/src/overlay.cpp` integration.
- **Blockers / risks:** No local build or in-game runtime is available in this session. Eclipse's hook set may not block every side effect; do not treat it as automatically safe. The implementation must be verified in-game later.
- **Relevant commit:** `58ae622a4378dae0a0a76ca58f339f946f89e693` adds the research document; checklist update follows.

## Session log

### 2026-11-25 — Eclipse trajectory reference reviewed
- Inspected Eclipse Menu's public `ShowTrajectory.cpp` from `Uproxide/EclipseMenu` and `Srivanthv/eclipsemenu`.
- Inspected current Geode 2.2081 `PlayerObject.cpp` binding implementation.
- Recorded hidden-player simulation, real collision/update reuse, and Eclipse's hook-based side-effect suppression strategy.
- Documented isolation risks and GeoDan implementation requirements in `docs/trajectory-eclipse-reference.md`.
- Removed the provisional scalar-physics model and overlay because they were less faithful and not tested.
- No build, automated test, or in-game verification run.

## Session log

- `58ae622a4378dae0a0a76ca58f339f946f89e693` — add Eclipse trajectory reference analysis.
- `8b9fe931c209064fe2dd003c2db9e52fcd9b1502` — remove provisional overlay.
- `0f55eddaaad40880dc41af2a48ba0d674ccbbf69` — remove provisional test.
- `23318b3eb897df3be80187ea51aead6ad0d4bda3` — remove provisional scalar model.

_Add newest entries first. Include date/time and relevant commit SHA._
