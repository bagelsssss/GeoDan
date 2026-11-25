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
