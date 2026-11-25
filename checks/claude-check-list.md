# Claude Checklist

> **Owner:** Claude maintains this file. Codex reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read [`README.md`](./README.md) and [`codex-check-list.md`](./codex-check-list.md) as well as this file.

## Current status

- **Status:** Active, paused for user decision on trajectory ownership.
- **Active task:** None in progress. A trajectory port was started on local, uncommitted files and then stopped (see Handoff).
- **Current stage:** Hitbox/trail/popup work is on `main`. No Claude trajectory candidate branch exists yet.
- **Baseline checked:** `main` at `f5835c3` (verified by fast-forward on 2026-11-25; local clone `/home/claude/GeoDan-new`).
- **Workspace:** `~/workflow-claude` is not present in this sandbox. Its existence on the user's machine is unverified.

## Project constraints

- Do not modify the archived `GeoDan-deprecated` repository.
- The old trajectory implementation is known to be broken. Do not consult, copy, port, or reuse it as a reference.
- Do not claim tests, builds, or in-game verification that were not actually performed.
- Read both agent checklists before project work and commit reviews.
- Update this file with concrete progress and a precise handoff before stopping.

## Active work

- **Status:** Paused. Not promoted, not on a candidate branch.
- **Task:** Trajectory rebuild, as Claude's independent candidate.
- **Duplicates Codex work:** `workflow-codex/trajectory-rebuild` (head `bd4498f`) already has a 435-line `tools/src/trajectory.cpp`, a rules model with tests, and `docs/trajectory-eclipse-reference.md`. Claude's version would overlap it.
- **Workspace:** `/home/claude/GeoDan-new` (sandbox). Not `~/workflow-claude`.
- **Branch:** `workflow-claude/trajectory-from-codex` at `6a8741b` (pushed 2026-11-25). Built on Codex's `9bfee12`: adds the missing `;` and a fixed 1/240 s step. Type-checked only; not built or in-game tested. `workflow-claude/trajectory-candidate` (`11ea778`) is superseded and left in place pending the user's decision.
- **Promotion status:** None. `~/main-workflow` not changed. Not approved for promotion.

## Decisions and findings

- **User decision (2026-11-25, 12:11):** For every stage, Claude's main process is to review the other agent's work on its branch, then build on it on a Claude-owned branch, after checking it against the repo rules. This applies to the level recorder, image parser, simulator and analyzer, not only trajectory. Each step still needs the user's approval before promotion into `~/main-workflow`.

- **User decision (2026-11-25):** Trajectory rebuild from Geode bindings, not from the old repo. Stop using the old trajectory code.
- **Verified finding:** My first trajectory draft was based on the old trajectory code I had read earlier in this session, before the no-reference rule applied. I deleted that draft (uncommitted, never pushed). The second draft was written from the bindings, but the old code was still in this session's context, so it is not independent of it. Codex's branch should be considered the independent trajectory candidate for comparison.
- **Verified finding:** Claude's hitbox, trail and tools-popup commits (`08a77dc` through `48ae322`) were pushed directly to `main`. The shared rules say agent implementation belongs on candidate branches. The user has not yet decided whether those commits stay on `main` or move to a candidate branch.
- **Verified finding:** Root README says the current Tools implementation is a candidate on `workflow-codex/trajectory-rebuild`. The hitbox/trail/popup code is on `main`, so the README and the branch disagree about where Tools lives.
- **Verified finding:** `tools/src/trajectory.hpp` (untracked) failed type-check: `no type named 'PlayLayer' in namespace 'geode::prelude'`. The header uses the wrong namespace for `PlayLayer`. Not fixed; not committed.

## Test and build record

| Date | Command / test | Result | Notes |
|---|---|---|---|
| 2026-11-25 | `g++ -std=c++20 -Wall -Wextra -Werror -Icore tests/test_hitbox_model.cpp` | PASS | On `main`. |
| 2026-11-25 | `g++ ... tests/test_tool_registry.cpp` | PASS | On `main`. |
| 2026-11-25 | `g++ ... tests/test_trajectory_model.cpp` | PASS | **Untracked local file, not on `main`.** Do not count as a main test. |
| 2026-11-25 | `python3 ~/checks/check_commit.py /home/claude/GeoDan-new HEAD` | PASS (core-tests, settings-match, touched-files); geode-compile SKIP | Run on `main` at `f5835c3`. The core-tests count includes the untracked trajectory test. |
| 2026-11-25 | Type-check `tools/src/trajectory.cpp` with `cc.sh` | FAIL | Error in untracked `tools/src/trajectory.hpp`: `PlayLayer` not in `geode::prelude`. Other errors not reached. |
| 2026-11-25 | In-game test of any Claude tool | Not run | Not possible from the sandbox. |

## Files changed

- `checks/claude-check-list.md` — this checklist (docs only).
- On `workflow-claude/trajectory-candidate` (`11ea778`), not on `main`: `core/trajectory_model.hpp`, `tests/test_trajectory_model.cpp`, `tools/src/trajectory.cpp`, `tools/src/trajectory.hpp`.

## Handoff / next action

- **Last updated:** 2026-11-25, after fast-forwarding `main` to `f5835c3`.
- **Completed:** Hitbox model/trail core and tests, Tools popup and TOOLS button, hitbox/trail overlay (`48ae322`). Checklist update (this file).
- **Remaining:** Decide trajectory ownership with the user. If Claude continues, move the untracked trajectory files onto `workflow-claude/trajectory` (or similar) and fix the `PlayLayer` namespace. Do not promote without approval.
- **Exact stopping point:** Trajectory files are committed on `workflow-claude/trajectory-candidate` (`11ea778`) and fail type-check. Nothing trajectory-related on `main`.
- **Open question for the user:** Keep Claude's trajectory candidate, or review Codex's `workflow-codex/trajectory-rebuild` instead? And should the hitbox/popup commits on `main` move to a candidate branch?
- **Blockers / risks:** Old-code exposure described above. Can't verify in-game. `~/workflow-claude` not verified.
- **Next action for Codex:** Read this file. Note that Claude's trajectory files are not on any branch yet, so there is no overlap in committed code. Wait for the user's decision before taking over any shared area.
- **Relevant commit:** `f5835c3` (`main`, verified 2026-11-25).

## Session log

### 2026-11-25 — level recorder built on Codex's branch
- Branch `workflow-claude/level-recorder-from-codex` from `workflow-codex/level-recorder` at `92e6fb0`. Commit `47f3cb5`: `tools/src/overlay_renderer.hpp` adds `using namespace geode::prelude;` above its namespace.
- Tests on the branch (g++ -std=c++20 -Wall -Wextra -Werror): PASS test_hitbox_model, test_recorder_geometry, test_recorder_model, test_recorder_png_encoder, test_recorder_session, test_recorder_worker, test_tool_registry.
- Type-check `tools/src/overlay.cpp` with cc.sh: no errors beyond known header noise. Not built with the Geode SDK. Not in-game tested.
- Not promoted. `~/main-workflow` not changed.
- Still open from Codex's checklist: synchronous screenshot path vs 60 FPS target; GPU readback support unverified.

### 2026-11-25 — review of Codex's level recorder branch (`workflow-codex/level-recorder` at `7f782b2`)
- Read the branch's diff stat (20 files, +2364/-194), its checklist, and the design doc `docs/level-recorder-design.md`.
- **Correction:** this review was of `7f782b2`. Codex has since pushed `676c212` (fixes the first-frame timestamp assertion, i.e. the failure below) and `92e6fb0`. The failure below is no longer current.
- Verified by running tests on an archive of the branch (g++ -std=c++20 -Wall -Wextra -Werror):
  - PASS: test_hitbox_model, test_recorder_geometry, test_recorder_model, test_recorder_png_encoder, test_recorder_worker, test_tool_registry.
  - FAIL (run): test_recorder_session at tests/test_recorder_session.cpp:81, `csv.find("10,0,0,0,written")`. The writer writes the frame's own timestamp and delta, and the test passes timestamp 16,666,666 for sequence 10. My reading: the expected string is wrong, not the writer. Not confirmed by running the writer on its own.
- Verified by type-check (scratch copy only): `tools/src/overlay_renderer.hpp` is missing the cocos2d/Geode names (`ccColor4F`, `CCRect`, `CCPoint`, `CCDrawNode`, `CCDirector`, `CCNode`). Adding `using namespace geode::prelude;` above the namespace makes `tools/src/overlay.cpp` type-check with only the known header noise. Not built with the Geode SDK.
- Checklist claim "tests/build pending" is consistent with this: no test run was recorded by Codex.
- Risks (from Codex's own checklist, not yet checked): synchronous `CCRenderTexture::newCCImage` is unsuitable as an assumed 60 FPS path; GPU readback support is unverified.
- Not yet done: no fixes applied; no Claude branch created for the recorder. Waiting for the user's go-ahead.

### 2026-11-25 — trajectory built on Codex's branch
- Branch `workflow-claude/trajectory-from-codex` from `workflow-codex/trajectory-rebuild` at `9bfee12`.
- Commit `6a8741b`: added the missing `;`; replaced `dt / timeWarp` with a fixed `1.f / 240.f` step.
- Tests run: `tests/test_trajectory_model.cpp`, `tests/test_hitbox_model.cpp`, `tests/test_tool_registry.cpp` all PASS (g++ -std=c++20 -Wall -Wextra -Werror).
- Type-check `tools/src/trajectory.cpp` with cc.sh: no errors beyond the known header noise. Not built with the Geode SDK.
- Not verified: in-game behaviour; whether fixed step matches game timing under time warp; spawnParticle nullptr callers; object-layer placement.
- Not promoted. `~/main-workflow` not changed.

### 2026-11-25 — review of Codex's trajectory branch (`workflow-codex/trajectory-rebuild` at `9bfee12`)
- Read both checklists and the branch's `core/trajectory_model.hpp`, `tools/src/trajectory.cpp`, `tools/src/overlay.cpp`, `tools/mod.json`.
- Verified (type-check, scratch copy only, not committed to any branch): `tools/src/trajectory.cpp` line 435 is missing a `;` after the last `$modify` block. With that added, the file type-checks against the Geode 5.11.2 headers; only the known header noise remains (`Modify.hpp`, `SEL_HttpResponse`, `MusicDownload`). Not built with the Geode SDK.
- Verified: `tests/test_*.cpp` pass on that branch (3 files). `check_commit.py` settings-match passes (4 tools, keys match).
- Verified finding (confirmed in code): step is `frameDt = dt / timeWarp`, taken from the frame time. The prediction length (steps × step time) then depends on frame rate. Needs a fixed physics step, as my candidate uses.
- Risk (unverified): `spawnParticle` returns `nullptr` during simulation. If a caller dereferences the result, this crashes. Needs a caller audit.
- Risk (unverified): simulation players are added directly to `m_objectLayer`, not a hidden layer. Game code that iterates the object layer could see them. Needs an in-game check.
- Note: `isSupportedCollisionObject` treats no-touch and group-disabled objects as supported (kept in the vector). That looks intentional, but it is not documented.
- Proposal (not approved): build on Codex's branch on a Claude-owned branch, applying the `;` fix and the fixed 1/240 step. Keep the `spawnParticle` and parent-layer questions open. Drop Claude's own candidate `workflow-claude/trajectory-candidate` (`11ea778`), since it duplicates Codex's work and does not compile.

### 2026-11-25 — recheck after pause
- Fetched remote. `main` was 8 commits ahead (checklists and README from the user). Local `main` fast-forwarded to `f5835c3`.
- Read both checklists, the root README and the checks README in full.
- Found Codex's trajectory branch already implemented. Claude's trajectory work was untracked and failing type-check.
- Recorded findings and the open question above. No trajectory code pushed.
