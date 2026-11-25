# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read [`README.md`](./README.md) and [`claude-check-list.md`](./claude-check-list.md) as well as this file.

## Current status

- **Status:** Active — first game-facing hold/release simulator and renderer are implemented in the isolated candidate; Geode build and in-game validation remain outstanding.
- **Active task:** Implement a safe, GeoDan-owned trajectory simulator using verified Geometry Dash 2.2081 behavior.
- **Current stage:** Game-facing simulator, collision safety filters, lifecycle ownership, and renderer added; validation pending.
- **Current branch:** `workflow-codex/trajectory-rebuild`.
- **Latest verified HEAD:** `9bfee125b1a0c8a413522fe0025ae0a195c78de8`.

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
- **User-approved implementation direction:** Use Eclipse Menu Show Trajectory as an architectural/technical reference, but keep all GeoDan implementation code original and independently reviewed.
- **CI policy:** GitHub Actions must be reserved for meaningful candidate checkpoints, not every commit.

## Test and build record

| Date | Command / test | Result | Notes |
|---|---|---|---|
| 2026-11-25 | Source/repository inspection | Completed | Re-read shared guide and both checklists; verified branch HEAD and source tree. No local checkout/runtime available. |
| 2026-11-25 | `g++ -std=c++20 -Wall -Wextra -Werror` trajectory model + guard test | Passed | Compiled and ran a temporary local copy of the committed header/test source; output: `trajectory model + simulation guard: all checks passed`. This validates only the pure data model, not Geode integration. |
| 2026-11-25 | Geode build / in-game test | Not run | The current session has no verified local checkout or Geode SDK/runtime. Must be done in the user's local environment. |
| 2026-11-25 | 2.2081 binding audit | Inspected | Checked GeometryDash.bro declarations for player creation/update/collision, button state, PlayLayer lifecycle, collision filtering, trigger, particle, event, and side-effect hook signatures. This does not reveal all binary-internal side effects. |
| 2026-11-25 | Candidate source/config review | Partial pass | Confirmed CMake globs `tools/src/*.cpp`; validated `tools/mod.json` parses as JSON. No Geode compilation was possible. |
| 2026-11-25 | GitHub Actions | Not run | Workflow remains `workflow_dispatch` only; no automatic builds were triggered. |

## Files changed

- `core/trajectory_model.hpp` — independent hold/release path state, bounded sample storage, stop reasons, reset semantics, and nested thread-local RAII simulation guard.
- `tests/test_trajectory_model.cpp` — pure C++ checks for branch isolation, stopping, reset, and bounds.
- `tools/src/trajectory.hpp` / `tools/src/trajectory.cpp` — hidden simulation-player lifecycle, live-state copying, hold/release stepping through real GD collision/update calls, dual-player predictions, conservative unsupported-interaction/moving-object stops, side-effect hooks, and path drawing.
- `tools/src/overlay.cpp` — updates trajectory after PlayLayer post-update; resets on restart and cleans up before quit.
- `tools/mod.json` — clarifies the hidden-copy behavior and conservative stop policy.
- `docs/trajectory-eclipse-reference.md` — updated implementation status and remaining validation risks.

## Handoff / next action

- **Last updated:** 2026-11-25.
- **Completed:** Added the pure model and a first game-facing implementation: two hidden simulation players are reused for hold/release branches and both live players in dual mode; the game collision/update routines feed bounded paths; the overlay draws those paths; lifecycle cleanup is wired to reset/quit.
- **Remaining:** Build against Geode 5.11.2 / GD 2.2081; resolve any compiler/API mismatches; test side-effect isolation in-game; validate cube/ship/wave/ball/UFO/robot/spider/swing, slopes, portals, triggers, dual, timewarp, death/restart/quit; profile the cost; inspect whether rotating/moving geometry or other shared effects require additional guards.
- **Exact stopping point:** Remote candidate branch `workflow-codex/trajectory-rebuild`, HEAD `9bfee125b1a0c8a413522fe0025ae0a195c78de8`. No Geode build, GitHub Actions run, or in-game test has been performed.
- **Next action:** User should fetch this branch into `~/workflow-codex` and build with the configured Geode SDK; return compiler output for correction. Keep `~/main-workflow` untouched pending user selection.
- **Blockers / risks:** The bindings confirm signatures but do not expose every binary-internal side effect. Hooks block known deaths, triggers, gravity flips, rings, game events, particles, and selected effects. The collision candidate list is filtered and restored, and known moving objects terminate the branch. This is not yet proof of complete isolation; audio, rotation, object mutations, and mode-specific behavior still require in-game audit. Local Codex workspace and Geode runtime are unavailable in this session.
- **Relevant commits:** `f356d04c8f4f427080e7a95832656aa6aa2faf36` (trajectory model + test); `97ff42c7f181c299c3d9576bcef71353f40a3d44` (manual-only Actions trigger); `63b70def0fc7e22a1c7656f0fc001466fe3e1a5d` (nested simulation guard); `5ed93339ca7238c60864b508d5b451fe87afd970` / `88136d0eada41db6000a1af9e044f9a6a60ee4b3` (runtime interface/implementation); `9bf6a3df0e506e8a765b6aab72ed75f0d7f4cab0` (overlay integration); `9bfee125b1a0c8a413522fe0025ae0a195c78de8` (latest docs/config/checklist state).

## Session log

### 2026-11-25 — first game-facing trajectory implementation
- Re-read the shared guide and both checklists; Claude's checklist still records no active work.
- Verified the isolated candidate branch and audited Geometry Dash 2.2081 binding declarations for the methods/fields used by the simulator.
- Added `tools/src/trajectory.cpp` and `.hpp`: hidden simulation players, live-state copy, two branches, real collision and player updates, dual-player support, collision candidate filtering/restoration, conservative stops for unsupported and known moving objects, selected side-effect suppression, and rendering.
- Connected the helper to `PlayLayer::postUpdate`, `resetLevel`, and `onQuit` in `tools/src/overlay.cpp`.
- Updated tool descriptions and research status. `tools/mod.json` parses as valid JSON.
- No Geode build, Actions run, or in-game validation was performed. The candidate must be treated as experimental until those checks are completed.

### 2026-11-25 — trajectory model foundation
- Re-read `checks/README.md`, both agent checklists, trajectory research notes, current overlay, settings, and CMake integration.
- Verified remote candidate branch HEAD was `80fc253d386d515f4fb51bea0dfc280927368d2f`.
- Added pure data structures for two independent prediction paths, bounded sample count, terminal reasons, and reset behavior.
- Added a standalone C++ test source and compiled/ran it with `g++ -std=c++20 -Wall -Wextra -Werror`; all checks passed.
- Changed the candidate workflow to `workflow_dispatch` only, so pushes no longer automatically consume Actions minutes. The workflow itself was not run.
- Added a nested RAII simulation context using thread-local nesting depth. Compiled and ran the full equivalent pure-C++ assertion set with `g++ -std=c++20 -Wall -Wextra -Werror`; all checks passed. No Geode build or in-game test was performed.
