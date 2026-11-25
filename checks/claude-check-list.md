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

### 2026-11-25 — recorder live integration: scoped, not started (user said go ahead)
- Found: the recorder branch has no live pipeline. The writer (`SessionWriter`), worker (`RecorderWorker`), geometry adapter (`snapshotGeometry`) and portrait target exist, but nothing connects them to PlayLayer. Codex's checklist says PlayLayer integration waits for a local Geode build and visual check.
- Found: pixel readback is not implemented on the recorder branch. The SDK bindings do contain `CCRenderTexture::newCCImage` (synchronous). Codex calls that unsuitable for a 60 FPS path. Asynchronous readback is unverified.
- Found: the `level-recorder` tool is still marked Planned in `core/tool_registry.hpp`, so the menu cannot enable it.
- Decision needed (user): accept a synchronous readback at 60 FPS (unverified, may drop frames), or wait for the local build check. Also: whether to enable `level-recorder` in the menu now, disabled by default.
- No code written for this step. Branch unchanged.

### 2026-11-25 — image parser review and build-on (`workflow-codex/image-parser` at `a042690`)
- Read the branch diff (6 files, +805/-11: `core/image_parser.hpp`, `tests/test_image_parser.cpp`, docs, workflow). Portable code only; no game files.
- Verified: `tests/test_image_parser.cpp` did NOT compile: lines 96 and 98 used `"\\"width\\":360"`, which ends the string literal early (6 occurrences in those two lines). Fixed.
- Verified after fix: test_image_parser builds with g++ -std=c++20 -Wall -Wextra -Werror and prints "Image Parser tests passed". Asserts enabled.
- Integration check (scratch copy, not committed): a session written by the recorder's `SessionWriter` (from `workflow-claude/level-recorder-from-codex`) with a real 360x1080 PNG is accepted by `parseSession`. A fake 16-byte PNG is rejected as `missing_or_wrong_sized_png`, which is the intended behaviour.
- Branch: `workflow-claude/image-parser-from-codex` at `04bf467`. Not promoted; `~/main-workflow` unchanged.
- Still open (from Codex's checklist): recorder sidecar integration into the live recorder, and the Analyzer adapter. Neither is implemented.
- Codex's checklist on the branch has stale sections ("Last updated: Not initialized", "Next action for Claude"). Flagged to the user; not edited by Claude.

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

### 2026-11-25 — level recorder wired live (`workflow-claude/recorder-live-from-codex`, commit `860be16`)
- User instruction: "its fine, you can implement it now" (accepts the synchronous readback risk; recorder in the menu, off by default).
- Branch base: `origin/workflow-claude/level-recorder-from-codex` at `47f3cb5`.
- Added `tools/src/recorder_live.hpp/.cpp`: `LiveRecorder` per PlayLayer attempt. Starts when `level-recorder` is on; renders the portrait target each paced frame (`FrameTimeline`), copies one RGBA frame into a `PixelBufferPool` lease with `newCCImage(true)`, queues with `tryPush`; the worker encodes and writes. Stops on `setting_off`, `level_restart` (resetLevel), `user_quit` (onQuit), `layer_destroyed`. Output: `<save dir>/recordings/session_<unix ns>/`.
- Registry: `level-recorder` → Available (5 available tools); `tests/test_tool_registry.cpp` count updated. `mod.json`: `level-recorder` setting, default false. `main.cpp`: new RECORDER section.
- `overlay.cpp`: recorder update placed after overlay drawing, before the portrait-preview early return; `resetLevel` and `onQuit` hooks stop the session.
- Corrections found while writing: `m_isDead` is on `PlayerObject`, not `PlayLayer` (fixed to `m_player1->m_isDead`); `LiveRecorder()` moved out of line because `Session` is incomplete in the header; `using namespace geode::prelude` moved above the portrait header include.
- Verified: all 7 core test files pass (g++ -std=c++20 -Wall -Wextra -Werror). Type-check of `recorder_live.cpp`, `overlay.cpp`, `main.cpp` against 2.2081 headers: only the known SDK noise (`MusicDownloadManager.hpp` SEL_HttpResponse; `Modify.hpp` string_view), which the untouched base `overlay.cpp` also shows.
- `check_commit.py` on `860be16`: core-tests PASS, touched-files PASS, geode-compile SKIP, **settings-match FAIL**: `portrait-preview-test` is in mod.json but not in the registry. The same failure exists on base `47f3cb5` (checked in a temporary worktree). It is a temporary diagnostic setting, not a tool. Not changed by me; needs a decision: allowlist diagnostic settings in the checker, or move the test setting out of mod.json when it is removed.
- Not verified: any build with the Geode SDK; in-game recording; the synchronous `newCCImage` cost at 60 FPS; whether `PlayLayer::onQuit` and the restart path behave as assumed; whether the Fields destructor runs when the layer is destroyed; that pausing does not break the timeline (it will record missed intervals); stop blocks the game thread while the queue drains (up to queue capacity of PNG encodes).
- Not promoted. `~/main-workflow` not changed. Branch pushed; no PR opened.

### 2026-11-25 — BUILDME.md research (findings only; no file created, no source changed)
- Task from user: independently verify build/test commands for candidate branches. Output returned to user in chat; `BUILDME.md` not created.
- Heads verified via `git ls-remote` on this date: main `5b65cd3`; workflow-codex/trajectory-rebuild `bd4498f`; workflow-claude/trajectory-from-codex `6a8741b`; workflow-codex/level-recorder `a1563a3`; workflow-claude/level-recorder-from-codex `47f3cb5`; workflow-claude/recorder-live-from-codex `860be16`; workflow-codex/image-parser `44b8fdb`; workflow-claude/image-parser-from-codex `04bf467`; workflow-codex/dan-analyzer `2085263`. Several Codex heads moved after my earlier reviews; those newer commits have not been reviewed by me.
- Portable tests run in detached worktrees on Ubuntu 24.04, g++ 13.3, `-std=c++20 -Wall -Wextra -Werror`, one executable per test file: all PASS on main, codex/trajectory-rebuild, claude/trajectory-from-codex, codex/level-recorder, claude/level-recorder-from-codex, claude/recorder-live-from-codex, codex/image-parser, claude/image-parser-from-codex. FAIL on codex/dan-analyzer: `tests/test_dan_analyzer_model.cpp:229` assertion (`band.valid && tierIndex == 1 && subdivision == MidHigh`) aborts (exit 134). Codex's checklist says this suite was not run.
- Type-check (`cc.sh`, Geode headers, `LIM=200`): codex/trajectory-rebuild `tools/src/trajectory.cpp:435` "expected ';' after struct" (still broken at head). claude/trajectory-from-codex trajectory.cpp clean. recorder-live `recorder_live.cpp` clean. Others clean beyond SDK noise.
- Codex trajectory head still uses `frameDt = dt / timeWarp` (frame-time step); fixed on claude/trajectory-from-codex only.
- README on main: contract-test command references `shared/` and `tests/test_tools_contract.cpp`; neither exists on main. Stale.
- Codex checklist on main still says "Awaiting Codex's first entry"; real Codex logs live on the branches. Stale on main.
- Geode CLI not installed in this sandbox; no Geode build was run. `geode build --platform windows` from Linux is unverified.
- Not changed: source, branches, `~/main-workflow`. Worktrees removed.

### 2026-11-25 — Windows build and in-game results, review plan (Claude)
- Scope: updates the record after the first Windows build of combined work. Nothing merged to main; no source changed on main.

**Build (user's PC, Arch, Geode CLI 4.0.1, SDK 5.11.2 at ~/.local/share/Geode/sdk, g++ 16.2.1):**
- `workflow-claude/recorder-live-from-codex` (`860be16`): `geode build --platform windows --config RelWithDebInfo --ninja` succeeded and installed `local.geodan-tools.geode`.
- `workflow-claude/combined-progress` at `9acfeb0`: failed. `trajectory.cpp` hooked `PlayerObject::playBurstEffect`, which is `win inline` in the 2.2081 bindings. Geode's static_assert stops this.
- `workflow-claude/combined-progress` at `a9f2856`: removed that hook; build succeeded (exit 0), packaged and installed.
- My earlier type-checks used the macOS bindings (gen-mac). They did not catch the Windows inline error. Corrected: earlier "clean" type-check results for trajectory were not valid for Windows.

**Windows-binding check (class-by-class against bindings/2.2081/GeometryDash.bro):**
- Codex trajectory head (`bd4498f`): hooked methods checked per class. Only PlayerObject::playBurstEffect is inline on Windows. Codex's head still contains that hook; Codex must remove it.
- A name-only grep gave false positives (incrementJumps matched other classes); per-class checking is required.

**In-game results so far (user report):**
- Menu on the installed mod (not the combined build): shows HITBOXES, TRAJECTORY, COMING NEXT with "Level recorder: Records the level with the tools on. Next." That text is Codex's level-recorder branch (`a1563a3`). Trajectory switch on, no predicted path visible. Codex's level-recorder branch has no trajectory code, so the switch does nothing.
- Combined build after the fix: not yet confirmed in game by the user.
- Recorder (live, `860be16`): creates session folders under `…/GeometryDash/geode/mods/local.geodan-tools/recordings/`. Two sessions: one `complete` with `stop_reason: level_restart`, one still `recording`. Both have `frames_written: 0`, frames.csv header only, no PNGs. Capture never queued a frame. Cause not known: no GeoDan lines in the Geode logs were found, and the log-path check was not yet run.
- Probable causes to check in code (not yet verified): readback returning null or a size mismatch; the render returning false; the player focus returning nothing. The live recorder is my code and the bug is mine to fix.

**Decisions and requests:**
- User asked to stop in-game testing of the recorder until Codex's review is done.
- User asked Codex to review all its implementations (prompt written; Codex to run it). Codex's branches become the base for my next work; my own branches are to be removed by the user after review (not deleted by me).
- Proposed deletion (user's decision, not executed): `workflow-claude/trajectory-candidate` (11ea778), `level-recorder-from-codex` (47f3cb5), `recorder-live-from-codex` (860be16), `trajectory-from-codex` (6a8741b), `image-parser-from-codex` (04bf467), `combined-progress` (a9f2856). SHAs are recorded here for recovery.
- main's popup/hitbox work stays unless the user asks otherwise.

**My next steps (after Codex's review report):**
1. Review each Codex implementation against its branch and the Windows bindings.
2. Create `workflow-claude/improve-on-codex` (name to confirm) from Codex's reviewed head.
3. Fix the live recorder's zero-frame bug on top of Codex's base; fix the trajectory's inline hook and frame-time step if Codex has not.
4. Run the portable tests per file and the Windows build; report results to the user. No in-game claims until the user confirms.

**Not verified:** in-game trajectory behaviour; in-game recorder behaviour; any Codex branch built on Windows except `recorder-live-from-codex` (`860be16`) and `combined-progress` (`a9f2856`).

### 2026-11-25 — alpha prototype decisions (user)
- User decision: build the alpha from Codex's reviewed code. Wait for Codex's self-review report before writing any alpha code.
- User decision: the alpha stays on a repo candidate branch. Promotion into ~/main-workflow needs a separate, explicit user approval after testing.
- Scope proposed by Claude and not yet confirmed by user: hitboxes, unclassified outlines, hitbox trail, working trajectory, level recorder writing frames the image parser accepts. Dan analyzer out of scope (test failing).
- Estimate given: about 3–5 hours of Claude work, plus game sessions from the user. Start after Codex's review report.
- Status: waiting on Codex's review. No alpha code written yet.

### 2026-11-25 — branch restructure (user decision)
- User decisions: Claude is the builder for the remaining mod work; prune Codex's branches but keep a Codex master; build the rest of the mod on Claude's master; then work on the recorder.
- Clarified: Claude cannot run the Windows Geode build here. The user runs builds on their PC and pastes output; Claude prepares the branch and commands.
- Created `workflow-codex/master` at `e5937fc`: `workflow-codex/combined-tools` (`278afd7`) plus `workflow-codex/dan-analyzer` (`24a5aea`) merged. Conflicts kept the combined-tools versions of README, registry, registry test, mod.json and main.cpp (the analyzer branch predates trajectory). Codex checklist keeps both logs verbatim. Per-file portable tests (10 files) all PASS.
- Archived Codex branches (copies, no history lost): `archive/codex-trajectory-rebuild` (3cc7391), `archive/codex-level-recorder` (d7d1a07), `archive/codex-image-parser` (795d579), `archive/codex-dan-analyzer` (24a5aea), `archive/codex-combined-tools` (278afd7).
- Deleting the old `workflow-codex/*` names (except master) was refused by the remote (HTTP 403). Not retried. User to delete on GitHub or allow it.
- Created `workflow-claude/master` at `e5937fc` (same as workflow-codex/master).
- Not mine: `alpha-branch` at `f80478a` appeared on the remote; not touched.
- Next: Windows build of `workflow-claude/master` (user runs), then Claude's build-the-rest work, then the recorder.
