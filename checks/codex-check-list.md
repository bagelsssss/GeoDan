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


## 2026-11-25 — Codex full self-review (Trajectory)

**Review status:** Windows-inline hook and fixed-step defects corrected. Portable tests passed. Geode Windows build and in-game behavior are not verified.

- Reviewed branch: workflow-codex/trajectory-rebuild
- Implementation/test head: aeb559b4aa1c8b7042f9c8264cdf24f9268d7951
- Branch base: main snapshot 6a2d5a1ffd2905911ac4f26c8a3829bcfb4b9bfd
- Commits since branch base: 35. Full ordered commit list and diffs: https://github.com/bagelsssss/GeoDan/compare/6a2d5a1ffd2905911ac4f26c8a3829bcfb4b9bfd...aeb559b4aa1c8b7042f9c8264cdf24f9268d7951
- Main was read at feb19b3e68e9c1996e673248a7cd006cd630c7a1; the trajectory work was founded on the earlier base above.
- No changes to main, ~/main-workflow, Claude-owned branches, or GeoDan-deprecated.

### Files changed relative to the branch base

- .github/workflows/geodan-core-tests.yml — manual trigger plus a narrowly path-filtered one-time self-review test trigger; each test is an independent step.
- checks/codex-check-list.md — trajectory development log and this review.
- core/trajectory_model.hpp — bounded hold/release paths, terminal reasons and nested simulation guard.
- docs/trajectory-eclipse-reference.md — implementation research and design constraints.
- tests/test_trajectory_model.cpp — path bounds, terminal state and RAII guard checks.
- tools/mod.json — false-by-default trajectory setting; remains registered as an Available tool on this branch.
- tools/src/overlay.cpp — invokes trajectory from PlayLayer update and resets/cleans it on lifecycle changes.
- tools/src/trajectory.cpp — hidden-player simulation, collision filtering, side-effect suppression and path rendering.
- tools/src/trajectory.hpp — PlayLayer-facing trajectory lifecycle declarations.

### Geode 5.11.2 / GD 2.2081 Windows hook audit

Checked the exact declaring class blocks in bindings/2.2081/GeometryDash.bro from geode-sdk/bindings revision d14587bf6c08b9114b5605ec6a3c42cc16b02de6 (blob 1d194f26246c69adf609a136d0d8056a4702a739). The methods below have a Windows address and are not inline:

| Modify class | Hooked method | Windows binding |
|---|---|---|
| PauseLayer | customSetup() | win 0x37c870 |
| PlayLayer (overlay) | postUpdate(float) | win 0x3b4ba0 |
| PlayLayer (overlay) | resetLevel() | win 0x3b8eb0 |
| PlayLayer (overlay) | onQuit() | win 0x3babb0 |
| PlayLayer (trajectory safety) | destroyPlayer(PlayerObject*, GameObject*) | win 0x3b39d0 |
| PlayLayer (trajectory safety) | playEndAnimationToPos(cocos2d::CCPoint) | win 0x3aba00 |
| GJBaseGameLayer | canBeActivatedByPlayer(PlayerObject*, EffectGameObject*) | win 0x2178c0 |
| GJBaseGameLayer | flipGravity(PlayerObject*, bool, bool) | win 0x212b00 |
| GJBaseGameLayer | playerTouchedRing(PlayerObject*, RingObject*) | win 0x217e40 |
| GJBaseGameLayer | gameEventTriggered(GJGameEvent, int, int) | win 0x231ff0 |
| GJBaseGameLayer | spawnParticle(const char*, int, cocos2d::tCCPositionType, cocos2d::CCPoint) | win 0x2402e0 |
| GJBaseGameLayer | collisionCheckObjects(PlayerObject*, gd::vector<GameObject*>*, int, float) | win 0x214960 |
| EffectGameObject | triggerObject(GJBaseGameLayer*, int, gd::vector<int> const*) | win 0x4a5f30 |
| GameObject | playShineEffect() | win 0x1993b0 |
| PlayerObject | incrementJumps() | win 0x38cba0 |
| PlayerObject | ringJump(RingObject*, bool) | win 0x398c00 |
| PlayerObject | playSpiderDashEffect(CCPoint, CCPoint) | win 0x395170 |
| PlayerObject | playBumpEffect(int, GameObject*) | win 0x39fa50 |
| PlayerObject | playDeathEffect() | win 0x37ef10 |
| PlayerObject | playCompleteEffect(bool, bool) | win 0x384020 |
| PlayerObject | playerDestroyed(bool) | win 0x397d40 |
| HardStreak | addPoint(cocos2d::CCPoint) | win 0x2b9800 |

Found and fixed: PlayerObject::playBurstEffect() is win inline in the PlayerObject block and cannot be hooked on Windows. Its $modify method was removed. It is no longer present in the candidate.

Found and fixed: the final HardStreak $modify block had no terminating semicolon; it now ends with the required semicolon. Rechecked the final source: no missing semicolon at the former line 435.

### Fixed timestep

- Replaced frameDt = dt / timeWarp with constexpr float kSimulationStepSeconds = 1.f / 240.f.
- Render dt is only checked for invalid/non-positive values; the prediction step itself no longer varies with render frame time or timeWarp.
- With the default 180 samples, the nominal simulated horizon is 179 steps after the initial point (approximately 0.746 s). The model hard maximum is 600 samples.
- This meets the requested fixed-step behavior, but whether 1/240 s reproduces GD physics under all frame rates and time-warp states is not established without game testing.

### Test record

All tests were run separately on GitHub Actions Ubuntu 26.04.1 at implementation/test head aeb559b4aa1c8b7042f9c8264cdf24f9268d7951. All steps completed with exit code 0.

| Test | Exact command | Exit | Stdout |
|---|---|---:|---|
| test_hitbox_model.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_hitbox_model.cpp -o /tmp/geodan-hitbox_model-review && /tmp/geodan-hitbox_model-review | 0 | hitbox model + trail: all checks passed |
| test_tool_registry.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_tool_registry.cpp -o /tmp/geodan-tool_registry-review && /tmp/geodan-tool_registry-review | 0 | tool registry: all checks passed |
| test_trajectory_model.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_trajectory_model.cpp -o /tmp/geodan-trajectory_model-review && /tmp/geodan-trajectory_model-review | 0 | trajectory model + simulation guard: all checks passed |

Workflow: https://github.com/bagelsssss/GeoDan/actions/runs/45558625333

### What works, what is broken, what remains unverified

- Portable PASS: trajectory path state, independent hold/release paths, sample bounds, terminal reasons, RAII simulation-guard nesting, hitbox model and tool registry.
- Source-level correction: no Windows-inline hooks remain in trajectory's $modify blocks; no playBurstEffect hook; final HardStreak modify block is terminated.
- Architecture: prediction runs hidden PlayerObject copies through game collision/update routines, filters unsupported interaction objects, restores the collision candidate vector and draws separate hold/release paths.
- Unverified risk — isolation: hidden simulation players are attached to the live m_objectLayer; game code iterating that layer may observe them. No runtime test established complete isolation.
- Unverified risk — side effects: spawnParticle returns nullptr during simulation; downstream call paths have not been proven safe when it returns null. The selected hooks do not prove every possible game side effect is suppressed.
- Unverified risk — simulation fidelity: copied player state, collision ordering, moving-object coverage, collision filters, dual behavior, and the 1/240 step have not been validated against actual gameplay.
- Geode Windows build: NOT RUN / not verified.
- In-game trajectory test: NOT RUN in this review. No visible path, collision prediction accuracy or absence of gameplay side effects is claimed.

### Settings/registry

- hitboxes, unclassified, hitbox-trail, and trajectory are Available and have matching false-by-default settings.
- level-recorder and macro-bot are Planned.
- portrait-preview-test is absent from this branch.
- Registry test confirms four Available tools.

### Review commits made

- 09ccbac2af7f97abb7beb33b03d5f1db220b5dc7 — remove Windows-inline playBurstEffect, fix final modify semicolon, and switch prediction to a fixed 1/240 s step.
- aeb559b4aa1c8b7042f9c8264cdf24f9268d7951 — update the test workflow with separate per-file commands and run the tests.

Next action: user-side Geode 5.11.2 Windows build followed by controlled in-game validation of visibility, collisions, side effects, reset and quit lifecycle.
