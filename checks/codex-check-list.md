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
