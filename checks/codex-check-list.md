# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read [`README.md`](./README.md) and [`claude-check-list.md`](./claude-check-list.md) as well as this file.

## Current status

- **Status:** Active — isolated Image Parser candidate; not approved for promotion.
- **Active task:** Implement and validate the Classic-only Image Parser against Recorder Session v1.
- **Current stage:** Portable session reader, section grouper, and provisional AABB-clearance parser implemented. Claude reviewed the candidate, fixed a test string-escape bug on a Claude-owned branch, ran the parser tests successfully, and performed a scratch integration check against the Recorder SessionWriter. The same test fix is now on this Codex branch. No current local run is claimed.
- **Baseline at checklist creation:** `main` — `8ce48311b51acae5bff35a1e117847b77df70f13` (Claude checklist creation commit; verify current HEAD before relying on it).

## Project constraints

- Do not modify the archived `GeoDan-deprecated` repository.
- The old trajectory implementation is known to be broken. Do not consult, copy, port, or reuse it as a reference.
- Do not claim tests, builds, or in-game verification that were not actually performed.
- Read both agent checklists before project work and commit reviews.
- Update this file with concrete progress and a precise handoff before stopping.

## Active work

- **Status:** Active — isolated Image Parser candidate; not approved for promotion.
- **Task:** Parse complete Classic Recorder Session v1 recordings, validate frame/timestamp integrity, group chronological images, and measure physical player-to-hazard clearance.
- **Remote branch:** `workflow-codex/image-parser`
- **Branch base:** `main` at `f5835c3d1291884e25e9384c3f70b0694cb0a7a0`.
- **Latest implementation/documentation commit before this checklist update:** `408c2e05c49938fa4c6aeac6628d4bc68b015af4`.
- **Local workspace:** User's `~/workflow-codex` was not accessed. Source was edited on the isolated remote branch and mirrored into `/tmp/geodan-parser` only for compilation/tests.
- **Promotion status:** Experimental only. No approval to promote into `~/main-workflow`.
- **Main-workflow changed:** No.
- **Previous separate candidate:** `workflow-codex/trajectory-rebuild` remains separate and untouched by this parser task.

### Parser implementation

- `core/image_parser.hpp` reads and validates Recorder Session v1 manifest/CSV, PNG signature and IHDR dimensions, and `hitboxes.csv` geometry sidecar.
- Groups persisted frames chronologically in sections of at most ten images, splitting at phase boundaries.
- Measures per-frame minimum Euclidean AABB clearance across player/hazard hitboxes; one minimum clearance sample per frame. Clearance is normalized to a 360-pixel reference width.
- Tracks sequence/missed-interval discontinuities, queue drops, unsupported geometry, and unclassified frames separately. Missing/unsupported geometry is never treated as zero clearance.
- `docs/image-parser-design.md` specifies the required hitbox sidecar. The current Recorder candidate now writes `hitboxes.csv` through `SessionWriter`; Claude's scratch integration check reports a real 360×1080 PNG session was accepted by `parseSession`. Live PlayLayer capture still is not connected.
- 60/120/240 FPS windows, Nerves, skillsets, Memory, and the Analyzer schema-v3 adapter remain unimplemented/unknown. No formula or telemetry is invented.
- Manual-only workflow added at `.github/workflows/geodan-image-parser-tests.yml`; it has not been dispatched.

## Decisions and findings

_Add dated, evidence-backed entries. Label each as a user decision, verified finding, or proposal._

## Test and build record

| Date | Command / test | Result | Notes |
|---|---|---|---|
| — | — | Not recorded | Do not infer test results. |

## Files changed

_Record exact repository-relative paths and a short description._

## Handoff / next action

- **Last updated:** Not initialized.
- **Completed:** No Codex work recorded yet.
- **Remaining:** Populate this section when work begins and before handing off.
- **Exact stopping point:** Not recorded.
- **Next action for Claude:** Read this file and the Claude checklist; coordinate before taking over overlapping work.
- **Blockers / risks:** None recorded.
- **Relevant commit:** `8ce48311b51acae5bff35a1e117847b77df70f13` (baseline at creation; verify current HEAD).

## Session log

### 2026-11-25 — isolated trajectory branch created
- Created `workflow-codex/trajectory-rebuild` from current `main` at `6a2d5a1ffd2905911ac4f26c8a3829bcfb4b9bfd`.
- Added kickoff status to this checklist on the candidate branch (commit `b381803c0bddf021cb7754f556a57fbd58d96d45`).
- User requested that branch workflow and current branch status also be documented in both repository READMEs and in the checks on `main`.
- The root README, `checks/README.md`, and this checklist are shared workflow documentation on `main`. No trajectory code or `~/main-workflow` changes.
- User is away from their Arch PC; local workspace setup is deferred.



### 2026-11-25 — Image Parser portable foundation
- Read the shared workflow guide and both current main-branch agent checklists before starting. Reviewed the Recorder Session v1 contract/schema and the Analyzer schema-v3 evidence model/design.
- Created isolated branch `workflow-codex/image-parser` from main `f5835c3d1291884e25e9384c3f70b0694cb0a7a0`. Candidate is experimental and remains separate from main and `~/main-workflow`.
- Added `core/image_parser.hpp`: complete-session manifest/CSV validation; frame sequence/timestamp/delta checks; PNG signature/IHDR dimension validation; required hitbox sidecar parsing; phase-aware chronological sections of at most ten persisted images; AABB clearance measurement; mean/median/minimum/p10; 360-pixel reference-width normalization; explicit unsupported/missing evidence and gap/drop accounting.
- Added `tests/test_image_parser.cpp` with synthetic session fixtures covering section sizing, phase splits, clearance statistics, resolution normalization, missing sidecar rejection, unsupported geometry, and wrong PNG dimensions.
- Added `docs/image-parser-design.md` defining the hitbox sidecar contract and limitations; added `.github/workflows/geodan-image-parser-tests.yml` with manual-only trigger.
- Local portable test command: `g++ -std=c++20 -Wall -Wextra -Werror -I/tmp/geodan-parser/core /tmp/geodan-parser/tests/test_image_parser.cpp -o /tmp/geodan-image-parser-tests && /tmp/geodan-image-parser-tests` — PASSED.
- Sanitizer command: `g++ -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I/tmp/geodan-parser/core /tmp/geodan-parser/tests/test_image_parser.cpp -o /tmp/geodan-image-parser-tests-asan && /tmp/geodan-image-parser-tests-asan` — PASSED.
- No GitHub Actions dispatch, Recorder build, Geode build, real capture parse, Analyzer integration, or in-game test was performed.
- Critical dependency: the current Recorder candidate writes manifest/frames.csv/PNG but does not emit the required `hitboxes.csv`. The parser is therefore not yet end-to-end usable on existing sessions. Next step is to implement a recorder-side geometry sidecar producer using actual collision geometry and verify coordinate mapping against the captured PNG.
- Timing windows, Nerves, and skillset classification are deliberately left Missing until telemetry and formulas are specified; 60 FPS imagery cannot establish sub-frame 120/240 FPS timing evidence.
- Files changed: `core/image_parser.hpp`, `tests/test_image_parser.cpp`, `docs/image-parser-design.md`, `.github/workflows/geodan-image-parser-tests.yml`, candidate-branch `README.md`, this Codex checklist.
- No user-local workflow directory was assumed or changed. `main` and `~/main-workflow` were not changed.



### 2026-11-25 — Claude parser review and test-fix cross-reference
- Read the updated main-branch Claude checklist. Claude created `workflow-claude/image-parser-from-codex` at `04bf4674e66c08e56b939ce7cb316f755b792d38`, based on Codex parser revision `a0426901f160943c097cce2ea64657d0d3b4fc0c`.
- Claude found two malformed C++ string literals in `tests/test_image_parser.cpp` (the width-field replacement test) and fixed the extra escaping. I inspected Claude's exact two-line patch and applied the same fix to this Codex branch; current Codex parser head before this checklist update is `408c2e05c49938fa4c6aeac6628d4bc68b015af4`.
- Claude reports `tests/test_image_parser.cpp` compiled with `g++ -std=c++20 -Wall -Wextra -Werror` and printed `Image Parser tests passed`, with assertions enabled, on the Claude branch. This Codex branch contains the same parser source and exact test fix, but I did not run it locally.
- Claude also reports a scratch integration check: a session written by `SessionWriter` from `workflow-claude/level-recorder-from-codex`, with a real 360×1080 PNG, was accepted by `parseSession`; a fake 16-byte PNG was rejected as expected. Verified that the `core/recorder_session.hpp` and `core/recorder_worker.hpp` blob SHAs on that Claude branch match the current Codex Recorder sidecar writer/worker. This is useful integration evidence, but it is a scratch check, not a committed integration test or in-game capture.
- Current Recorder branch `workflow-codex/level-recorder` is at `1b25ddb68d8ec1cb92f4504b217753857ebada17`; the sidecar writer exists, while the latest game-facing geometry adapter and validation tests still need Claude's fresh review/run.
- No changes to `main`, `~/main-workflow`, or Claude-owned branches. No local parser test run or GitHub Actions dispatch by Codex.
- Next: retain Claude's fix in the Codex candidate; after the Recorder live producer is wired, run the parser against an actual recorded session and implement the Analyzer adapter.


## 2026-11-25 — Codex full self-review (Image Parser)

**Review status:** Portable tests passed. Parser is a sidecar-driven, provisional geometry parser, not a pixel-recognition engine. Geode build and in-game behavior are not verified.

- Reviewed branch: workflow-codex/image-parser
- Implementation/test head: 326e2c4c3881d62708ab57a82fde9ddccc6d4c06
- Branch base: main snapshot f5835c3d1291884e25e9384c3f70b0694cb0a7a0
- Commits since branch base: 23. Full ordered commit list and diffs: https://github.com/bagelsssss/GeoDan/compare/f5835c3d1291884e25e9384c3f70b0694cb0a7a0...326e2c4c3881d62708ab57a82fde9ddccc6d4c06
- Main was read at feb19b3e68e9c1996e673248a7cd006cd630c7a1; this candidate started from the earlier base above.
- No changes to main, ~/main-workflow, Claude-owned branches, or GeoDan-deprecated.

### Files changed relative to the branch base

- .github/workflows/geodan-image-parser-tests.yml — original manual-only parser test workflow.
- .github/workflows/geodan-codex-review-tests.yml — one-time, path-filtered review test workflow with a separate step for every test file.
- README.md — pipeline status and Image Parser description.
- checks/codex-check-list.md — parser development log and this review.
- core/image_parser.hpp — manifest/timeline/PNG-IHDR/geometry-sidecar validator, section grouper, AABB clearance statistics and explicit missing/unsupported state.
- core/tool_registry.hpp — review correction: trajectory marked Planned because this parser-only candidate does not include the trajectory implementation.
- docs/image-parser-design.md — Session v1, hitbox sidecar, AABB measurement and evidence limitations.
- tests/test_image_parser.cpp — synthetic completed-session parser tests.
- tests/test_tool_registry.cpp — test that the branch has three Available tools.
- tools/mod.json — removed unavailable trajectory setting; remaining tool settings are false by default.
- tools/src/main.cpp — moved unavailable trajectory out of Available menu sections and into Planned/Coming Next.

### Geode hook audit

The only $modify classes are PauseLayer and PlayLayer. Checked their exact class blocks in bindings/2.2081/GeometryDash.bro from geode-sdk/bindings revision d14587bf6c08b9114b5605ec6a3c42cc16b02de6 (blob 1d194f26246c69adf609a136d0d8056a4702a739):

| Modify class | Hooked method | Windows binding |
|---|---|---|
| PauseLayer | customSetup() | win 0x37c870, not inline |
| PlayLayer | postUpdate(float) | win 0x3b4ba0, not inline |
| PlayLayer | resetLevel() | win 0x3b8eb0, not inline |

No inline hook found. Image Parser code itself is portable and is not wired into the mod.

### Test record

All tests were run separately on GitHub Actions Ubuntu 26.04.1 at implementation/test head 326e2c4c3881d62708ab57a82fde9ddccc6d4c06. All steps completed with exit code 0.

| Test | Exact command | Exit | Stdout |
|---|---|---:|---|
| test_hitbox_model.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_hitbox_model.cpp -o /tmp/geodan-hitbox_model-review && /tmp/geodan-hitbox_model-review | 0 | hitbox model + trail: all checks passed |
| test_image_parser.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_image_parser.cpp -o /tmp/geodan-image_parser-review && /tmp/geodan-image_parser-review | 0 | Image Parser tests passed |
| test_tool_registry.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_tool_registry.cpp -o /tmp/geodan-tool_registry-review && /tmp/geodan-tool_registry-review | 0 | tool registry: all checks passed |

Workflow: https://github.com/bagelsssss/GeoDan/actions/runs/45558630024

### What the parser actually does

- It reads manifest.json, frames.csv, hitboxes.csv and checks that each referenced PNG has the expected signature and IHDR dimensions.
- It does not decode PNG pixel data, run computer vision, detect hazards from artwork, or infer collision geometry from pixels. Clearance measurements are computed from the Recorder's hitboxes.csv sidecar.
- It groups persisted frames chronologically into sections of at most ten images, split at phase changes.
- For valid player/hazard AABBs, it computes the minimum Euclidean rectangle gap per active frame and section/level mean, nearest-rank median, minimum and p10.
- It normalizes pixel distances to a 360-pixel reference width and keeps missing/unsupported evidence distinct from zero.
- The Recorder geometry adapter supplies provisional AABBs; rotated/circular shapes use envelopes, player bounds use the diagnostic inner rectangle, and lethal slope edges are unsupported. Exact engine collision geometry is not established.
- 60/120/240 FPS windows, Nerves, skillsets, Memory classification and the Analyzer schema-v3 adapter remain unimplemented/unknown.
- The Recorder branch still has no live frame producer, so this parser is not end-to-end usable from an actual live recording generated by the reviewed Recorder candidate. It can parse a completed session that already conforms to the documented v1 files and sidecar.

### Settings and registry

- hitboxes, unclassified, and hitbox-trail are the three Available tools and have matching false-by-default settings.
- trajectory, level-recorder, and macro-bot are Planned. No trajectory switch remains in this branch's mod.json.
- No portrait-preview-test setting exists on this branch.

### What works, what is broken, and what remains unverified

- Portable PASS: manifest/timeline checks, sequence/delta validation, PNG signature and dimensions, geometry-sidecar integrity, section splitting, normalized AABB clearance, and unsupported/missing evidence handling.
- Not implemented: pixel-level image analysis, actual hitbox recognition from pixels, timing-window analysis, Nerves formula, skillset classification, Memory classification and Analyzer adapter.
- Geode Windows build: NOT RUN / not verified.
- In-game test: NOT RUN in this review. No actual gameplay session from the live Recorder was parsed.
- Integration: no real Recorder→Parser end-to-end live run has been performed.

### Review commits made

- 8140355e040bb96024d18ae96ce6d39905240a4e — mark trajectory Planned in the parser-only component candidate.
- 5bd52fec31e93aa513368628ed57af0e026e1366 — begin removal of the unavailable trajectory setting.
- 2156054a988a8063735a1ffb33124105b2c9c17a — move trajectory to the Planned popup section.
- a9b52ab3aa8c49f526b478956ea9125c043cc683 — finish removal of the trajectory setting from mod.json.
- 4b45a5986f225284473c9454d3466d055ca6aa43 — align registry test with the three Available tools.
- 326e2c4c3881d62708ab57a82fde9ddccc6d4c06 — add one-time review workflow and run the parser test suite.

Next action: the parser needs a real completed Recorder session with the required sidecar and validated geometry. Pixel recognition and remaining Analyzer metrics must be designed as distinct future work; do not claim they are present.
