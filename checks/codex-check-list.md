# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Read `checks/README.md` and `checks/claude-check-list.md` before project work.

## Current status

- **Status:** Combined Tools candidate assembled; implementation and integration validation remain in progress.
- **Branch:** `workflow-codex/combined-tools`
- **Base:** latest `main` at `0e7a3a7fb6ac0995ad9d176d0c1ccd63b1b585bf`.
- **Promotion:** Not approved for `main` or `~/main-workflow`.
- **User decision:** Codex and Claude may contribute to this shared candidate. Coordinate before overlapping edits and preserve contributor provenance.
- **Local workspaces:** No local user workspace was accessed or modified.

## Source revisions merged

1. **Live Level Recorder first** — `workflow-claude/recorder-live-from-codex` at `860be16838963270fd6966bd1ee76d7ff3ec84a9`.
   - Merge commit: `487f4fee8a33c1f4f3e3225d4408d1767a57b0fb`.
   - Contains the Recorder portable model, PNG encoder, worker/session writer, hitbox sidecar, geometry adapter, portrait target/preview, live PlayLayer capture bridge, and Tools menu/registry changes.
2. **Fixed-step Trajectory second** — `workflow-claude/trajectory-from-codex` at `6a8741b00e91f5da487e27af2af74a33e601e824`.
   - Merge commit: `bfbd2edcacef6384c9b7efb5137f4f71ec578b19`.
   - Contains the trajectory model, fixed 1/240-second prediction step, hidden simulation players, collision filters, side-effect guards, renderer, tests, and technical documentation.
   - The broken Codex branch `workflow-codex/trajectory-rebuild` at `bd4498f51d43ff24f5c0bebb9cbbb83f2801b5d1` was not merged.
3. **Image Parser core/tests third** — `workflow-codex/image-parser` at `44b8fdb0eed5a740ee63776b94d9ab44bbf22c35`.
   - Merge commit: `1d78cba49df7bfcc87db2eff7564a810dd4e463c`.
   - Only `core/image_parser.hpp` and `tests/test_image_parser.cpp` were added. The Parser was not wired into the Geode mod; parser docs/workflow were not imported.
4. **Dan Analyzer excluded** — `workflow-codex/dan-analyzer` at `2085263aa9e6bbd6d45045cf8a039e9fb8e3e4cd` was not merged, including its known failing test and its code/docs.

## Conflict resolutions

- **`tools/src/overlay.cpp`:** Kept the Recorder's rendering and portrait-preview implementation in one `$modify(..., PlayLayer)` class in this file. Added the Trajectory include. `postUpdate` order is hitbox drawing, trail drawing, `updateGeoDanTrajectory`, `f.recorder.update`, then portrait preview. `resetLevel` stops Recorder first, calls the parent reset, resets Trajectory, then resets the trail. `onQuit` stops Recorder, cleans up Trajectory, then calls the parent quit hook. The Trajectory implementation's separate PlayLayer interception class in `trajectory.cpp` remains for simulation safety hooks.
- **`core/tool_registry.hpp`:** Retained the Recorder-live registry: hitboxes, unclassified, hitbox-trail, trajectory, and level-recorder are the five Available tools. Macro bot remains Planned.
- **`tests/test_tool_registry.cpp`:** Retained the total count of six definitions (five Available plus one Planned); added an explicit assertion that exactly five tools are Available. The `everythingOn` expectation remains five.
- **`tools/mod.json`:** Retained settings for the five Available tools, all default false, and retained `portrait-preview-test`, also default false. Settings were ordered as requested. `check_commit.py` was not changed.
- **`tools/src/main.cpp`:** Retained the Recorder-live implementation. Its section order was verified as `HITBOXES`, `TRAJECTORY`, `RECORDER`, `COMING NEXT`; no code edit was necessary.
- **Shared repository docs:** Latest `main` versions of `README.md`, `BUILDME.md`, `checks/README.md`, and `checks/claude-check-list.md` were preserved. Only this Codex checklist was updated.

## Test record

The requested command form for each test is:

```bash
g++ -std=c++20 -Wall -Wextra -Werror -I"$PWD/core" -I"$PWD" tests/<test-file>.cpp -o /tmp/geodan-<test-name>
/tmp/geodan-<test-name>
```

**Combined-branch tests were not run in this session.** The GitHub connector can inspect and commit remote files but does not provide a checked-out repository shell; the container could not resolve `github.com` when attempting to obtain a checkout (`curl: (6) Could not resolve host: github.com`). Therefore no C++ test stdout/stderr exists to report. Source-branch test reports are not counted as combined-branch test results.

| Test file | Result on this combined candidate | Exact output |
|---|---|---|
| `tests/test_hitbox_model.cpp` | NOT RUN | No output; compiler was not invoked |
| `tests/test_tool_registry.cpp` | NOT RUN | No output; compiler was not invoked |
| `tests/test_recorder_geometry.cpp` | NOT RUN | No output; compiler was not invoked |
| `tests/test_recorder_model.cpp` | NOT RUN | No output; compiler was not invoked |
| `tests/test_recorder_png_encoder.cpp` | NOT RUN | No output; compiler was not invoked |
| `tests/test_recorder_session.cpp` | NOT RUN | No output; compiler was not invoked |
| `tests/test_recorder_worker.cpp` | NOT RUN | No output; compiler was not invoked |
| `tests/test_trajectory_model.cpp` | NOT RUN | No output; compiler was not invoked |
| `tests/test_image_parser.cpp` | NOT RUN | No output; compiler was not invoked |

The Dan Analyzer test is excluded with its entire implementation. No Geode SDK build, GitHub Actions run, or in-game test was performed.

## Structural checks performed

- Verified selected remote source refs and exact SHAs before composition.
- Verified the combined overlay has one PlayLayer modify class in `overlay.cpp`.
- Verified the call positions in `postUpdate` are in the required order: Trail, Trajectory, Recorder, portrait preview.
- Parsed `tools/mod.json` and verified the five tool settings plus portrait preview all default to false.
- Verified the popup section order and the five Available registry entries plus Planned macro bot.
- Verified Parser files are present and no Analyzer model/test files were added.

These are repository-structure checks, not C++ compilation or runtime verification.

## Open items / known limitations

- **Portrait preview settings-match:** `portrait-preview-test` is not a registry tool and is known to trigger the `check_commit.py` settings-match failure. Decision remains pending; checker was not changed.
- **Trajectory runtime:** The fixed-step source correction is included, but prediction behavior, side-effect isolation, mode coverage, and performance remain unverified in-game.
- **Recorder performance:** The live Recorder uses synchronous `CCRenderTexture::newCCImage(true)` readback. Its cost and actual 60 FPS performance must be measured on the user's machine.
- **Recorder geometry:** Rotated/circular hazard bounds remain conservative AABB envelopes; unsupported geometry is flagged, not treated as empty/safe.
- **Parser integration:** Parser core/tests are present but not wired to the mod or validated against a live recorded session from this combined branch.
- **Analyzer:** Excluded by instruction. No Dan score assignment or calibration pipeline is included.
- **Geode build:** Not run. Only the user's configured SDK/runtime can confirm the mod build and in-game behavior.

## Handoff

- Run each portable test above separately on the user's machine and record exact output.
- Resolve the settings-match decision without modifying `check_commit.py` unless the user explicitly decides to do so.
- Build the Geode mod and test Recorder, Trajectory, and portrait preview in-game.
- Validate Parser against an actual Recorder session before any future Analyzer integration.
- Keep `main` and `~/main-workflow` unchanged unless promotion is separately approved.


## 2026-11-25 — combined-tools Windows-hook and main-sync review

**Task:** Fix workflow-codex/combined-tools for the known Windows-inline Geode hook, remove the unverified live Recorder bridge, merge current main, run each available portable test independently, and record the exact result.

### Branch and merge record

- **Branch:** workflow-codex/combined-tools
- **Starting head:** 290d1dc55131e9c7753a33e10c5d5b7d6cb25fda
- **Current main at task start:** 491c94612f19ba47601d94059898f2a14e8d87d5
- **Source/test head:** eabaadc6a21f5ebceb99cb490d998921eda7ff9c
- **Merge result:** merged current main using a normal two-parent merge commit, 31f37da609605571970ea1fba4abe82e8ec91223.
  - First parent: cleaned combined-tools source commit d0ee0cd74972660284de41b78b1daa61aecf3ef0.
  - Second parent: main at 491c94612f19ba47601d94059898f2a14e8d87d5.
  - This preserves the already-pushed history; no rebase or force-push was used.
- **Conflicts:** none. README.md, BUILDME.md, and checks/README.md had identical blob contents on the branch and main at merge. Main's updated checks/claude-check-list.md was incorporated unchanged from main (blob 747d07b39fe93b20fcc2f14594f4428315db11b9). The branch-owned checks/codex-check-list.md was retained and updated here. No shared README or BUILDME edits were needed.
- The compare against current main reports behind_by = 0. The candidate remains isolated and is not merged into main.
- main, ~/main-workflow, and GeoDan-deprecated were not modified. No old trajectory implementation was consulted, copied, or ported. No Claude branch was used as a source.

### Commits made for this task

1. d0ee0cd74972660284de41b78b1daa61aecf3ef0 — remove Windows-inline playBurstEffect hook, remove the inactive LiveRecorder bridge and align the Level Recorder registry state.
2. 31f37da609605571970ea1fba4abe82e8ec91223 — merge current main into combined-tools, retaining main's checklist update.
3. eabaadc6a21f5ebceb99cb490d998921eda7ff9c — add a per-file portable test workflow and run it.
4. This checklist update records the final review results and exact outputs.

### Changes made

- tools/src/trajectory.cpp — removed the PlayerObject::playBurstEffect() $modify hook and its parent call. The Windows binding declares that method win inline; it cannot be hooked by Geode on Windows. All other trajectory hooks were left unchanged.
- tools/src/overlay.cpp — removed the recorder_live.hpp include and namespace, the LiveRecorder field, the LiveRecorder::update call, and the stop calls from resetLevel and onQuit. Kept hitbox drawing, trail recording, trajectory calls, and portrait-preview diagnostics.
- tools/src/recorder_live.cpp — deleted.
- tools/src/recorder_live.hpp — deleted.
- tools/src/main.cpp — removed the now-empty RECORDER popup section classification. The planned Level Recorder now appears under COMING NEXT. This file had no direct LiveRecorder hook/reference before the change; the runtime hooks were in overlay.cpp.
- core/tool_registry.hpp — retained the level-recorder registry entry but changed its status from Available to Planned. Its blurb now states that the Recorder library is present but live capture is pending. Updated the ID comment to clarify that Available tools use their IDs as setting keys.
- tools/mod.json — removed the level-recorder toggle because its live producer was removed and the tool is Planned. Kept the four actual tool settings and the separate portrait-preview-test diagnostic setting, all default false.
- tests/test_tool_registry.cpp — changed Available-tool and countOn expectations from five to four. The Planned Level Recorder cannot be enabled even if a stored value says true.
- .github/workflows/geodan-combined-tools-review-tests.yml — added a path-filtered push/manual workflow with an independent compile-and-run step for every available test file.

### Level Recorder decision (user-directed)

The portable Recorder library remains in this branch: timing/queue model, geometry model and sidecar, PNG encoder, session writer, and worker. The live capture bridge is excluded because the live Recorder has been observed creating sessions with zero written frames; its live-capture fix is being handled separately. A library that cannot yet produce frames must not be presented as an Available in-game tool. Therefore:

- level-recorder remains in core/tool_registry.hpp as Planned.
- The live level-recorder setting is removed from tools/mod.json.
- The popup lists it under COMING NEXT, disabled.
- The library and its portable tests remain available for future integration.
- No live Recorder capture is claimed or tested here.

### Geode 5.11.2 / Geometry Dash 2.2081 Windows hook audit

Audited each $modify class and its hooked method against that method's declaration inside the matching class block in bindings/2.2081/GeometryDash.bro, from geode-sdk/bindings revision d14587bf6c08b9114b5605ec6a3c42cc16b02de6, blob 1d194f26246c69adf609a136d0d8056a4702a739.

All methods below have an explicit win address and are not inline:

| $modify class | Hooked method | Windows binding status |
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
| HardStreak | addPoint(CCPoint) | win 0x2b9800 |

The removed PlayerObject::playBurstEffect() declaration is win inline in the PlayerObject class block. It is no longer hooked or referenced in tools/src/trajectory.cpp. No other Windows-inline hook was found.

main.cpp also contains a custom onTools(CCObject*) callback inside the PauseLayer modify class; it is not a Geode hook override and is not included as a hooked method in the table.

### Trajectory fixed-step check

- Confirmed tools/src/trajectory.cpp still defines constexpr float frameDt = 1.f / 240.f.
- The same frameDt is passed to both Hold and Release branch simulations for both players.
- dt and timeWarp remain validated as live-state inputs, but neither changes the simulation step.
- This is a source inspection only; fixed-step fidelity against actual GD behavior is not runtime-verified.

### Per-file portable test record

Run on GitHub Actions Ubuntu 26.04.1 at source/test head eabaadc6a21f5ebceb99cb490d998921eda7ff9c, in workflow run https://github.com/bagelsssss/GeoDan/actions/runs/45640176083. Each test was compiled and executed as a separate step. The checkout-root equivalents of the requested include flags are -I"./core" -I".". Each step completed successfully with exit code 0.

| Test file | Exact command | Exit code | Program output |
|---|---|---:|---|
| tests/test_hitbox_model.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I"./core" -I"." "tests/test_hitbox_model.cpp" -o "/tmp/geodan-test-hitbox-model" && "/tmp/geodan-test-hitbox-model" | 0 | hitbox model + trail: all checks passed |
| tests/test_image_parser.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I"./core" -I"." "tests/test_image_parser.cpp" -o "/tmp/geodan-test-image-parser" && "/tmp/geodan-test-image-parser" | 0 | Image Parser tests passed |
| tests/test_recorder_geometry.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I"./core" -I"." "tests/test_recorder_geometry.cpp" -o "/tmp/geodan-test-recorder-geometry" && "/tmp/geodan-test-recorder-geometry" | 0 | recorder geometry: all checks passed |
| tests/test_recorder_model.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I"./core" -I"." "tests/test_recorder_model.cpp" -o "/tmp/geodan-test-recorder-model" && "/tmp/geodan-test-recorder-model" | 0 | recorder model: all checks passed |
| tests/test_recorder_png_encoder.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I"./core" -I"." "tests/test_recorder_png_encoder.cpp" -o "/tmp/geodan-test-recorder-png-encoder" && "/tmp/geodan-test-recorder-png-encoder" | 0 | recorder PNG encoder: basic structure checks passed |
| tests/test_recorder_session.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I"./core" -I"." "tests/test_recorder_session.cpp" -o "/tmp/geodan-test-recorder-session" && "/tmp/geodan-test-recorder-session" | 0 | recorder session: all checks passed |
| tests/test_recorder_worker.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I"./core" -I"." "tests/test_recorder_worker.cpp" -o "/tmp/geodan-test-recorder-worker" && "/tmp/geodan-test-recorder-worker" | 0 | recorder worker: queue drain, PNG output and finalization checks passed |
| tests/test_tool_registry.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I"./core" -I"." "tests/test_tool_registry.cpp" -o "/tmp/geodan-test-tool-registry" && "/tmp/geodan-test-tool-registry" | 0 | tool registry: all checks passed |
| tests/test_trajectory_model.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I"./core" -I"." "tests/test_trajectory_model.cpp" -o "/tmp/geodan-test-trajectory-model" && "/tmp/geodan-test-trajectory-model" | 0 | trajectory model + simulation guard: all checks passed |
| tests/test_dan_analyzer_model.cpp | ABSENT | N/A | No Analyzer test file exists on this branch; Dan Analyzer was not part of combined-tools. |

**Result:** 9/9 available tests PASS; 0 failed. The Dan Analyzer test is absent, not skipped after failure.

### Registry and settings consistency

- Registry entries marked Available: hitboxes, unclassified, hitbox-trail, trajectory.
- Every Available tool has a matching tools/mod.json setting. No missing Available setting.
- level-recorder and macro-bot are Planned and do not have settings.
- The only setting that is not a registry tool is portrait-preview-test. It is the expected temporary diagnostic preview setting, false by default.
- tools/mod.json parsed as valid JSON after the Level Recorder setting removal.
- Registry test expects four Available tools and passes.

### Final structural verification

At source/test head eabaadc6a21f5ebceb99cb490d998921eda7ff9c:

- tools/src/recorder_live.cpp and tools/src/recorder_live.hpp are absent.
- No LiveRecorder, recorder_live, recorder.update, or recorder.stop references remain in tools/src/main.cpp or tools/src/overlay.cpp.
- tools/src/main.cpp has no empty RECORDER section; planned Level Recorder appears in COMING NEXT.
- tools/src/trajectory.cpp has no playBurstEffect method or call.
- The fixed 1.f / 240.f step remains.
- tools/mod.json has four Available tool settings plus the one diagnostic exception.
- The branch contains no tests/test_dan_analyzer_model.cpp.

### Not verified

- **Geode Windows build: NOT RUN.** The source audit found and removed the known win inline blocker, and all portable tests pass, but no geode build --platform windows command was run. Do not claim that a Windows package/build has succeeded.
- **In-game behavior: NOT RUN.** Hitboxes, trail, trajectory visibility/collision fidelity, portrait preview, layer lifecycle and absence of gameplay side effects have not been validated in-game.
- **Recorder live capture: intentionally removed and NOT tested.** The retained Recorder library is portable code only, not a functional live capture tool.
- Performance, 60 FPS capture viability, actual Parser consumption of a live session, and Windows SDK integration remain unverified.

### Handoff

1. Build this exact candidate on the user's Geode 5.11.2 Windows toolchain.
2. If the build succeeds, perform the user's controlled in-game checks for hitboxes, trail, trajectory, portrait preview, reset and quit lifecycle.
3. Do not re-add the live Recorder until the zero-frame issue has been fixed and separately verified.
4. Keep the candidate isolated; no main merge or ~/main-workflow promotion was requested or performed.

<!-- merged from workflow-codex/dan-analyzer 24a5aea: its checklist entries, kept verbatim below -->
# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read README.md and claude-check-list.md as well as this file.

## Current status

- **Status:** Active — Classic-only Analyzer includes evidence aggregation plus score-scale, raw-section aggregation, calibration, and skillset classification primitives. Feature weights and a measured calibration curve remain pending.
- **Active task:** Define and validate the section scoring formula using actual Image Parser evidence and calibration levels.
- **Current stage:** Portable scoring/calibration foundation and judge skillset output added; expanded tests authored but not yet run.
- **Current branch:** workflow-codex/dan-analyzer.
- **Latest implementation commit before this checklist update:** 109139b73fe743d61a6b3f18836251a4ce5fa13f (additional timing-total and overlapping-section tests).

## Project constraints

- Do not modify the archived GeoDan-deprecated repository.
- Do not consult, copy, port, or reuse the old broken trajectory implementation.
- Do not claim tests, builds, or in-game verification that were not actually performed.
- Read both agent checklists before project work and commit reviews.
- Keep this candidate isolated; do not modify main-workflow or promote without explicit user approval.
- Keep GitHub Actions manual-only and use them sparingly.
- Prioritize measurement accuracy over tuning Dan labels.
- The score scale and label bands cover 1st–10th Dan, Alpha–Iota, and Kappa. Do not invent calibration anchors or feature weights.
- Unknown evidence is not zero; unsupported interactions are not safe/empty space.
- **User decision (2026-11-25): Platformer implementation and support are cancelled entirely. GeoDan is Classic-only.** Remove Platformer fields, paths, and tests rather than leaving dormant support.
- The Image Parser measures the successful recorded run from physical hitboxes; do not replay that run merely to remeasure its physics.
- Simulation is reserved for discovering and validating alternative swag routes.
- Harder swag routes must not increase difficulty; easier confirmed routes may reduce effective difficulty according to difficulty-bearing gameplay bypassed.

## Active work

- **Status:** Active — isolated analyzer candidate, not approved for promotion.
- **Workspace:** ~/workflow-codex (local checkout not verified; remote branch is the source edited in this session).
- **Branch:** workflow-codex/dan-analyzer, originally based on main at 5cfd2dc01f17761f77be4aee6970f8b1998b10f2.
- **Promotion state:** Experimental; not approved for ~/main-workflow.
- **Main workflow changed:** No.
- **Latest implementation commit:** 10cb7db3fe3df30292eb12175c8ecc9820ca65e9. The subsequent checklist handoff is committed separately. Current compare against main before the final handoff update: 44 commits ahead, 1 behind. Candidate remains isolated.

## Decisions and findings

- **User decision:** Tools and Level Recorder may be marked mostly implemented but not tested. README records them as candidate implementations, not promoted/validated features.
- **User decision:** Bot search policy remains single.
- **User decision:** Memory threshold policy remains both. Exact semantics and numeric threshold are not guessed; visual obstruction and simulation unclear-path evidence are kept separate.
- **User decision:** Final Dan measurement policy remains both. Exact combination semantics are not guessed.
- **User decision:** Platformer is cancelled. The Analyzer schema is Classic-only.
- **User decision (2026-11-25):** The analyzed score itself is on a 0.00–50.00 scale. For every numbered Dan tier and Alpha–Iota: X.00–X.35 Low/Mid, X.35–X.65 Mid, X.65–next whole tier Mid/High. Kappa is 20.01–50.00; 20.00 remains Iota Mid/High.
- **User decision (2026-11-25):** Deadlocked is a designated middle anchor at 6.50 (6th Dan Mid). Its raw score is not known yet; the earlier example 120.43 was explicitly made up and must never be stored as data.
- **User decision (2026-11-25):** Main skillset categories are Overall, Learny, Precision, and High CPS. Sub-skillsets use the existing taxonomy; do not implement Memory classification yet.
- **Design decision:** Section raw-score contributions are signed and additive. A missing section contribution makes the raw level total incomplete. Feature weights and contribution-generation formula remain undefined until grounded in measured data.
- **Design decision:** Calibration uses monotonic piecewise-linear interpolation between at least two measured raw/analyzed anchor pairs, with no extrapolation. A single Deadlocked anchor cannot define the curve.
- **User decision:** Preserve the requested skillset taxonomy: Circles, Clicksync, Fast-Paced, Timings, Chokepoints, Unbalanced, Learny, Memory, High CPS, Gimmicky, Flow, Slow-Paced, Bossfight, Mirror, Nerve Control, Cube, Ship, Ball, UFO, Wave, Robot, Spider, Old Swing, New Swing, Duals, Overall.
- **User decision:** Image Parser groups about ten chronological images per section, measures physical hitbox clearances, retains section evidence, calculates 60/120/240 FPS windows and Nerves, and sends structured output to the Analyzer.
- **User decision:** Swag routes are alternative completable paths. Harder alternatives do not affect Dan; easier alternatives lower effective difficulty in proportion to difficulty-bearing gameplay bypassed.
- **Design interpretation:** The recorded successful route is a reference route for comparisons, not proof of creator intent.
- **Verified repository finding:** The original Analyzer branch had a Geode-free evidence model and no score formula. It included Classic and Platformer; the Platformer support has now been removed from the new schema.
- **Verified repository finding:** The trajectory candidate branch exists but is not built or validated in-game. It must not be treated as proof of route-search accuracy.
- **Physics evidence review:** Uploaded geodan-log(2).txt records a rotated-slope run with noclip=unverified. It is diagnostic evidence, not a validated physics oracle.
- **Design proposal:** A Dan score formula is deferred until parser evidence, route validation, and calibration data are available. The current judge returns readiness status, not a Dan.

## Test and build record

| Date | Command / test | Result | Notes |
|---|---|---|---|
| 2026-11-25 | g++ -std=c++20 -Wall -Wextra -Werror -I/tmp/geodan-analyzer /tmp/geodan-analyzer/test_dan_analyzer_model.cpp | Passed (earlier revision only) | This predates the new score/calibration/classification tests; it does not validate the current expanded suite. |
| 2026-11-25 | Expanded Dan score/calibration/classification tests | Not run | Must run the exact current branch test suite before treating this milestone as verified. |
| 2026-11-25 | GitHub Actions analyzer workflow | Not run | Workflow remains workflow_dispatch only; this session had no workflow-dispatch action. |
| 2026-11-25 | Geode build / in-game tests | Not run | No game integration or actual parser/route-search runtime exists. |

## Files changed

- core/dan_analyzer_model.hpp — adds weighted section aggregation, section-level distribution summaries, timing-window totals and coverage, Nerves aggregation, parser-to-section consistency checks, overflow guards, ordered/non-overlapping section validation, integral timing counts, unique route IDs/reference validation, and incomplete status for unquantified easier routes.
- tests/test_dan_analyzer_model.cpp — adds tests for two-section aggregation, weighted mean/median/p10, tightest clearance, frame-window totals, Nerves, idle exclusion, parser-summary mismatches, missing data, fractional timing counts, easier-route maximum bypass, harder-route non-inflation, and unquantified easier routes.
- docs/dan-analyzer-design.md — documents aggregation semantics and explicitly distinguishes section-level percentiles from exact frame-level percentiles.
- README.md — unchanged in this milestone; it already describes the Classic-only pipeline.
- .github/workflows/geodan-analyzer-tests.yml — unchanged; manual-only portable test workflow.

## Handoff / next action

- **Last updated:** 2026-11-25.
- **Completed:** Added weighted section aggregates for clearance and Nerves, section-level weighted median/p10 distributions, min/max, timing-window totals and coverage, maximum validated easier-route bypass fraction, and consistency checks against parser-level mean/minimum/timing/Nerves summaries. Added chronological non-overlap/range checks, fractional timing rejection, overflow guards, unique route IDs, exactly one reference route, and an insufficient-evidence gate for completed easier routes without validated bypass fractions. Expanded tests for two-section aggregation, weighted distributions, timing totals, Nerves, idle exclusion, parser-summary mismatch, incomplete evidence, fractional windows, mismatched timing totals, overlapping sections, easier/harder route comparison, and unquantified easier routes. Documented which statistics cannot be reconstructed exactly from section summaries.
- **Remaining:** Run the exact current branch test suite and fix any compile/test failures. Define feature transformations/weights with measured parser evidence, then populate additional verified anchors and evaluate calibration on held-out Classic levels. Add integration with the future Image Parser and Swag Route Detection. Define Nerves semantics and route-search coverage/termination evidence with the user.
- **Exact stopping point:** Code, tests, and documentation committed to the isolated remote branch. The new test suite was authored but NOT compiled or run in this session. No GitHub Actions run, Geode build, parser integration, route simulation, or Dan calibration.
- **Next action:** Run `g++ -std=c++20 -Wall -Wextra -Werror -Icore tests/test_dan_analyzer_model.cpp -o /tmp/geodan-dan-analyzer-tests && /tmp/geodan-dan-analyzer-tests` in the Codex checkout or manually dispatch the existing portable test workflow; fix any failures before extending the model.
- **Blockers / risks:** Image Parser and Swag Route Detection do not yet exist. Frame-window validity depends on actual recorder timestamp/frame precision. The simulator candidate is not validated. Nerves and bypass-fraction measurement definitions still need calibration. Whole-level frame median/p10 cannot be exactly reconstructed from section summary percentiles; the model deliberately reports section-level distributions separately.
- **Relevant commits:** 8582cf6e751f87db2be3e9c646dcbcbd5a4b7f88 (aggregation); e72f0f84236b9ea9e9e696346d5b5a7650d0ae33 (validation hardening); d620f39adcd7de5f9555a579408463c9cb973343 (tests); 40b4bbe06fa620c272ea3d46aa9e3e75c2d17a65 (design docs); 109139b73fe743d61a6b3f18836251a4ce5fa13f (additional verification tests); 653cdb4833711d368bcf8cf3c9cba190f247c0bc through 10cb7db3fe3df30292eb12175c8ecc9820ca65e9 (score/calibration/classification milestone).

## Session log

### 2026-11-25 — Classic-only Analyzer judge foundation
- Read the shared workflow guide and both agent checklists before editing.
- Compared workflow-codex/dan-analyzer against its recorded prior HEAD 943ed2f5d7bba22149184703fe3f17c8abc349c3; GitHub reported identical.
- Applied the user's project-wide decision to cancel Platformer: removed GameMode, primaryMode, per-segment mode, and Platformer test coverage from the Analyzer model.
- Reworked the Analyzer to consume Image Parser measurements for the recorded successful run: per-section clearance statistics, 60/120/240 FPS frame-window counts, and Nerves.
- Kept simulation evidence separate for Swag Route Detection. Added reference/alternative route types, completion state, relative difficulty, and a difficulty-bearing bypass fraction. Only validated completed easier routes are counted; harder routes do not increase difficulty.
- Added a judge result with Rejected, InsufficientEvidence, and ValidatedForCalibration states. This is not a Dan assignment formula.
- Updated README and design documentation to state Classic-only scope and distinguish Image Parser from route-search simulation.
- Compiled the locally reconstructed exact model/test contents with g++ -std=c++20 -Wall -Wextra -Werror; all checks passed.
- No main branch, main-workflow, local user workspace, Geode integration, or in-game behavior was changed.


### 2026-11-25 — Section aggregation and consistency verification
- Re-read the shared workflow guide and both agent checklists. Confirmed the candidate branch was identical to the recorded prior head before edits.
- Added section-level aggregate distributions: sample-count-weighted means, weighted median and p10 across section values, min/max, coverage counts, and represented samples.
- Added total 60/120/240 frame-window counts and coverage, weighted Nerves aggregation, and maximum validated bypass fraction across completed easier routes.
- Added parser-summary consistency checks for weighted clearance mean, minimum clearance, timing-window totals, Nerves weighted mean, and clearance sample count.
- Deliberately did not claim that a true frame-level median/p10 can be reconstructed from per-section summary statistics; the model reports section-level distributions separately.
- Hardened validation for chronological non-overlapping section ranges, source-frame bounds, strictly increasing section indexes, integral frame-window counts, duplicate route IDs, exactly one reference route, overflow, and completed easier routes with missing/unvalidated bypass fractions.
- Expanded portable tests for two-section aggregation, weighted distributions, timing totals, Nerves, idle exclusion, parser-summary mismatch, incomplete evidence, fractional windows, easier/harder route comparison, and unquantified easier routes.
- Updated design documentation. The test suite was authored but was NOT compiled or run in this session; no GitHub Actions dispatch action was available.
- No main branch, main-workflow, local user workspace, Geode runtime, Image Parser, or Swag Route simulator was changed.


- Additional tests: parser-level timing-window total mismatch and overlapping section sample ranges are explicitly rejected. These tests are authored but still unrun.


## 2026-11-25 — Codex full self-review (Dan Analyzer)

**Review status:** Portable test suite passed after correcting a floating-point boundary defect and unsupported-evidence classification. This is still an evidence/judging foundation, not a complete Dan scoring formula.

- Reviewed branch: workflow-codex/dan-analyzer
- Implementation/test head: 35042561622e0d1a0e222eb3b12614355ba93de8
- Branch base: main snapshot 5cfd2dc01f17761f77be4aee6970f8b1998b10f2
- Commits since branch base: 56. Full ordered commit list and diffs: https://github.com/bagelsssss/GeoDan/compare/5cfd2dc01f17761f77be4aee6970f8b1998b10f2...35042561622e0d1a0e222eb3b12614355ba93de8
- Main was read at feb19b3e68e9c1996e673248a7cd006cd630c7a1; this candidate started from the earlier base above.
- No changes to main, ~/main-workflow, Claude-owned branches, or GeoDan-deprecated.

### Files changed relative to the branch base

- .github/workflows/geodan-analyzer-tests.yml — original manual-only Analyzer portable test workflow.
- .github/workflows/geodan-codex-review-tests.yml — one-time, path-filtered review workflow with a separate compile-and-run step for every test.
- README.md — Classic-only Analyzer pipeline status.
- checks/codex-check-list.md — Analyzer design/development log and this review.
- core/dan_analyzer_model.hpp — Classic-only evidence validation, section aggregates, route evidence, raw contribution aggregation, calibration and skillset classification.
- core/tool_registry.hpp — review correction: trajectory marked Planned because this Analyzer-only candidate does not include trajectory code.
- docs/dan-analyzer-design.md — evidence contract, route semantics, aggregate and calibration limitations.
- tests/test_dan_analyzer_model.cpp — evidence, aggregate, route, calibration, score-band and classification tests, including the corrected boundary and unsupported-skill cases.
- tests/test_tool_registry.cpp — test that this component candidate has three Available tools.
- tools/mod.json — removed the unavailable trajectory setting.
- tools/src/main.cpp — moved trajectory out of Available sections into Planned/Coming Next.

### Geode hook audit

Only PauseLayer and PlayLayer $modify classes remain in the Tools shell. Checked the exact declaring class blocks in bindings/2.2081/GeometryDash.bro from geode-sdk/bindings revision d14587bf6c08b9114b5605ec6a3c42cc16b02de6 (blob 1d194f26246c69adf609a136d0d8056a4702a739):

| Modify class | Hooked method | Windows binding |
|---|---|---|
| PauseLayer | customSetup() | win 0x37c870, not inline |
| PlayLayer | postUpdate(float) | win 0x3b4ba0, not inline |
| PlayLayer | resetLevel() | win 0x3b8eb0, not inline |

The Analyzer itself is pure C++; it is not connected to a Geode hook or game layer.

### Test record

All tests were run separately on GitHub Actions Ubuntu 26.04.1 at implementation/test head 35042561622e0d1a0e222eb3b12614355ba93de8. All steps completed with exit code 0.

| Test | Exact command | Exit | Stdout |
|---|---|---:|---|
| test_dan_analyzer_model.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_dan_analyzer_model.cpp -o /tmp/geodan-dan_analyzer_model-review && /tmp/geodan-dan_analyzer_model-review | 0 | Dan analyzer aggregation and verification: all checks passed |
| test_hitbox_model.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_hitbox_model.cpp -o /tmp/geodan-hitbox_model-review && /tmp/geodan-hitbox_model-review | 0 | hitbox model + trail: all checks passed |
| test_tool_registry.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_tool_registry.cpp -o /tmp/geodan-tool_registry-review && /tmp/geodan-tool_registry-review | 0 | tool registry: all checks passed |

Workflow: https://github.com/bagelsssss/GeoDan/actions/runs/45558634606

### Known failure E — test assertion or model?

The assertion is correct; the model was wrong.

- The rating policy puts X.65 in Mid/High.
- The old implementation computed fraction = displayed - floor(displayed) and compared it with 0.65. Binary floating-point can represent 1.65 - 1.0 as slightly below 0.65, incorrectly classifying an exact 1.65 as Mid.
- Fixed ratingBandForScore to round to integer hundredths first and compare integer remainders. This preserves the policy and does not weaken the assertion.
- The same integer-boundary path handles 20.00 as Iota Mid/High and starts Kappa at 20.01.
- The formerly failing tests/test_dan_analyzer_model.cpp now compiles and runs successfully; output is shown above.

Additional model correction found during review: classifyMainSkillset and classifySubSkillsets previously could select evidence marked Unsupported if its positive-observation ratio was high. Both now explicitly exclude Unsupported evidence, with regression tests. This keeps unsupported evidence from producing a skillset tag.

### What works, what is broken, and what remains unverified

- Portable PASS: schema and evidence validation, active-section aggregation, timing-window totals, Nerves aggregation, route evidence, incomplete-evidence handling, raw-score section aggregation, calibration interpolation, rating-band boundaries, and skillset classification tests.
- Not a completed Dan scorer: no feature-weight formula or method to derive each section's raw contribution from parser measurements is implemented. Calibration anchors with actual measured raw scores are absent; Deadlocked is a designated 6.50 analyzed-score anchor with no invented raw score. No calibrated Dan can be assigned from a real level yet.
- Integration missing: no Image Parser→Analyzer adapter, no live Swag Route Search/Simulator integration, no level input pipeline.
- Memory: the two memory evidence fields are represented separately; Memory skillset classification remains deliberately excluded under the recorded project decision.
- Platformer: deliberately cancelled; Analyzer is Classic-only.
- Geode Windows build: NOT RUN / not verified.
- In-game test: NOT RUN in this review. The Analyzer is portable/pure C++ and has no in-game UI or runtime integration in this branch.
- Calibration quality: no held-out-level evaluation or real-level accuracy result is available.

### Settings and registry

- hitboxes, unclassified, and hitbox-trail are the three Available tools with matching false-by-default settings.
- trajectory, level-recorder, and macro-bot are Planned; trajectory setting removed from mod.json.
- No portrait-preview-test setting exists on this branch.

### Review commits made

- cbe5cab750647006e5d49fbf37bf879eece083af — mark trajectory Planned in Analyzer-only component candidate.
- 4551d93c4b975edd0656645bd5197ab5aba22d73 — begin removal of unavailable trajectory setting.
- a1dbf6314870c4e60ba917535997aadcc1783ac7 — move trajectory to Planned popup section.
- b164080d9ce4771976c79b9de4b0beb3f9d3088b — finish removal of trajectory setting from mod.json.
- 2eef2444d9d7035bfd3cf5d10633fc1ea85a8778 — exclude Unsupported evidence from skillset classification.
- 696b8e9bc801956e5b6667488ec1492001e9ae01 — add unsupported-evidence classification regression tests.
- 68cc2e81e44ee1501cef3906d166118b2fc78edd — align registry test with the three Available tools.
- 35042561622e0d1a0e222eb3b12614355ba93de8 — add one-time review workflow and run the test suite.

Next action: define the parser-to-analyzer adapter and evidence-grounded section scoring formula, then populate real measured calibration anchors before any accuracy claim.

## 2026-11-26 — Recorder output aligned with portrait preview

- **Branch:** `workflow-codex/recorder-preview-match`.
- **Base:** `workflow-codex/master` at `dc24b1851715d22ffbc7b107df650a78c5ccfddd`; comparison reports 0 commits behind.
- **Latest code/test commit:** `121d8a8b4205eb83a2271aef32f54d39b01c6765`.
- **Objective:** Make recorded PNG frames match the on-screen 360×1080 diagnostic portrait preview instead of capturing the full gameplay object layer (icon sprites and object art).
- **Source review finding:** The former Recorder called `renderAndRead(layer->m_objectLayer, ...)`, while the preview rendered a separate `previewRoot` containing diagnostic hitbox outlines, both ground lines, player rectangles and optional hitbox trail. The two paths also used different focus/zoom parameters.
- **Implementation:** Added a shared prepared portrait view (`previewRoot`, focus and scale) in `tools/src/overlay.cpp`. `preparePortraitContent()` builds the preview scene once per frame when either preview or recording is enabled. The visible preview and Recorder now render the same subtree with the same focus and scale. The Recorder no longer renders `m_objectLayer`.
- **Capture metadata:** `CaptureTransform` now uses the same focus and scale as the rendered diagnostic scene, keeping the hitbox sidecar mapped to the output image.
- **Readback hardening:** `PortraitCaptureTarget::renderAndRead()` checks the `glReadPixels()` GL error and restores the previous `GL_PACK_ALIGNMENT`; it does not copy stale scratch bytes after a readback error. Shared capture-view coordinates are checked for finite values, and camera dimensions use the render target's rounded point dimensions.
- **Tools UI:** Level Recorder is Available in the menu, default off. The portrait preview setting description now describes the diagnostic-only scene it previews.
- **Portable tests:** 9/9 PASS on GitHub Actions Ubuntu 24.04 at commit `121d8a8b4205eb83a2271aef32f54d39b01c6765`, run [45776881808](https://github.com/bagelsssss/GeoDan/actions/runs/45776881808). Tests: dan analyzer, hitbox/trail, image parser, recorder geometry/model/PNG/session/worker, and tool registry.
- **Not run:** No Geode Windows build and no in-game capture test. The repository MP4 binaries could not be decoded through the GitHub file connector in this session; the visual mismatch was identified from the actual two render paths in source. A user device build and side-by-side clip check remain required.
- **No changes:** `workflow-codex/master`, `workflow-claude/master`, `workflow-claude/recorder`, `main`, and `alpha-branch` were not modified.
- **Next action:** Build this exact candidate on the user's Arch/Geode 5.11.2 toolchain. Record a short capture with Level Recorder on and compare `frames/frame_*.png` or the resulting video against the visible portrait preview. Keep the feature branch isolated until that check passes.
