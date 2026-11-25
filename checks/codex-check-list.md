# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read [`README.md`](./README.md) and [`claude-check-list.md`](./claude-check-list.md) as well as this file.

## Current status

- **Status:** Active — prediction data model and nested RAII simulation guard implemented; game-facing simulation remains unimplemented.
- **Active task:** Implement a safe, GeoDan-owned trajectory simulator using verified Geometry Dash 2.2081 behavior.
- **Current stage:** Core prediction state/data model and scoped guard.
- **Current branch:** `workflow-codex/trajectory-rebuild`.
- **Current HEAD before this stage:** `80fc253d386d515f4fb51bea0dfc280927368d2f`.

## Project constraints

- Do not modify the archived `GeoDan-deprecated` repository.
- The old trajectory implementation is known to be broken. Do not consult, copy, port, or reuse it as a reference.
- Do not claim tests, builds, or in-game verification that were not actually performed.
- Read both agent checklists before project work and commit reviews.
- Keep this candidate isolated; do not modify `main-workflow` or promote without user approval.
- Use GitHub Actions sparingly: do not build every commit. Prefer deliberate validation checkpoints after a coherent milestone.

## Active work

- **Status:** Active — isolated candidate, not approved for promotion.
- **Workspace:** `~/workflow-codex` (local checkout not verified; remote branch is the source edited in this session).
- **Branch:** `workflow-codex/trajectory-rebuild`.
- **Promotion state:** Experimental; not approved for `~/main-workflow`.
- **Main workflow changed:** No.

## Decisions and findings

- **User decision:** Keep Codex and Claude implementations isolated; only user-selected changes may be promoted.
- **Verified repository finding:** `tools/mod.json` already contains a disabled-by-default trajectory setting.
- **Verified repository finding:** Gameplay overlay integration point is `tools/src/overlay.cpp`; source files are globbed by `tools/CMakeLists.txt`.
- **User-approved reference:** Eclipse Menu's public `ShowTrajectory.cpp` may be studied as technical reference. GeoDan must own its implementation; no wholesale copying.
- **Safety requirement:** The data model must not implement substitute physics. The future adapter must feed it positions from Geometry Dash's real update/collision routines only after side-effect isolation is audited.
- **Safety requirement:** Stop a branch on unsafe/unknown interactions rather than drawing a misleading continuation.
- **CI policy:** GitHub Actions must be reserved for meaningful candidate checkpoints, not every commit.

## Test and build record

| Date | Command / test | Result | Notes |
|---|---|---|---|
| 2026-11-25 | Source/repository inspection | Completed | Re-read shared guide and both checklists; verified branch HEAD and source tree. No local checkout/runtime available. |
| 2026-11-25 | `g++ -std=c++20 -Wall -Wextra -Werror` trajectory model + guard test | Passed | Compiled and ran a temporary local copy of the committed header/test source; output: `trajectory model + simulation guard: all checks passed`. This validates only the pure data model, not Geode integration. |
| 2026-11-25 | Geode build / in-game test | Not run | Must be done later in the user's local environment. |

## Files changed

- `core/trajectory_model.hpp` — independent hold/release path state, bounded sample storage, stop reasons, reset semantics, and nested thread-local RAII simulation guard.
- `tests/test_trajectory_model.cpp` — pure C++ checks for branch isolation, stopping, reset, and bounds.

## Handoff / next action

- **Last updated:** 2026-11-25.
- **Completed:** Added a physics-agnostic prediction data model. It deliberately does not claim to simulate Geometry Dash.
- **Remaining:** Audit exact 2.2081 hook/API declarations and side effects; implement RAII simulation context and player lifecycle; implement real player update/collision stepping; implement renderer and lifecycle cleanup; test locally and in-game.
- **Exact stopping point:** Core model and test source committed on isolated remote branch; no game-facing simulation code added.
- **Next action:** Continue with the side-effect guard and hidden-player lifecycle only after confirming all exact hook signatures and game call paths.
- **Blockers / risks:** Binding files expose only part of the game implementation; internal GD behavior requires careful in-game validation. Local Codex workspace and local build are unavailable in this session.
- **Relevant commits:** `f356d04c8f4f427080e7a95832656aa6aa2faf36` (trajectory model + test); `97ff42c7f181c299c3d9576bcef71353f40a3d44` (manual-only Actions trigger); `63b70def0fc7e22a1c7656f0fc001466fe3e1a5d` (nested simulation guard).

## Session log

### 2026-11-25 — trajectory model foundation
- Re-read `checks/README.md`, both agent checklists, trajectory research notes, current overlay, settings, and CMake integration.
- Verified remote candidate branch HEAD was `80fc253d386d515f4fb51bea0dfc280927368d2f`.
- Added pure data structures for two independent prediction paths, bounded sample count, terminal reasons, and reset behavior.
- Added a standalone C++ test source and compiled/ran it with `g++ -std=c++20 -Wall -Wextra -Werror`; all checks passed.
- Changed the candidate workflow to `workflow_dispatch` only, so pushes no longer automatically consume Actions minutes. The workflow itself was not run.
- Added a nested RAII simulation context using thread-local nesting depth. Compiled and ran the full equivalent pure-C++ assertion set with `g++ -std=c++20 -Wall -Wextra -Werror`; all checks passed. No Geode build or in-game test was performed.
