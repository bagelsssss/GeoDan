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
