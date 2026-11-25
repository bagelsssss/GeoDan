# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read README.md and claude-check-list.md as well as this file.

## Current status

- **Status:** Active — Classic-only Dan Analyzer judge foundation updated to consume Image Parser measurements and swag-route evidence; no calibrated Dan formula.
- **Active task:** Implement the portable judge/evidence contract before Image Parser integration.
- **Current stage:** Classic-only schema, parser evidence validation, and route evidence validation.
- **Current branch:** workflow-codex/dan-analyzer.
- **Latest implementation commit:** a5cff6e6a7f9a59b0bd68a87e27a448296676572 (README update; branch head should be verified before further edits).

## Project constraints

- Do not modify the archived GeoDan-deprecated repository.
- Do not consult, copy, port, or reuse the old broken trajectory implementation.
- Do not claim tests, builds, or in-game verification that were not actually performed.
- Read both agent checklists before project work and commit reviews.
- Keep this candidate isolated; do not modify main-workflow or promote without explicit user approval.
- Keep GitHub Actions manual-only and use them sparingly.
- Prioritize measurement accuracy over tuning Dan labels.
- Do not calibrate alpha–kappa in the initial analyzer pass.
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
- **Verified previous branch head:** 943ed2f5d7bba22149184703fe3f17c8abc349c3. A compare against the branch ref reported identical before edits.

## Decisions and findings

- **User decision:** Tools and Level Recorder may be marked mostly implemented but not tested. README records them as candidate implementations, not promoted/validated features.
- **User decision:** Bot search policy remains single.
- **User decision:** Memory threshold policy remains both. Exact semantics and numeric threshold are not guessed; visual obstruction and simulation unclear-path evidence are kept separate.
- **User decision:** Final Dan measurement policy remains both. Exact combination semantics are not guessed.
- **User decision:** Platformer is cancelled. The Analyzer schema is Classic-only.
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
| 2026-11-25 | g++ -std=c++20 -Wall -Wextra -Werror -I/tmp/geodan-analyzer /tmp/geodan-analyzer/test_dan_analyzer_model.cpp | Passed | Exact current candidate model/test contents were reconstructed locally and compiled; output: Dan analyzer judge model: all checks passed. |
| 2026-11-25 | GitHub Actions analyzer workflow | Not run | Workflow remains workflow_dispatch only. |
| 2026-11-25 | Geode build / in-game tests | Not run | No game integration or actual parser/route-search runtime exists. |

## Files changed

- core/dan_analyzer_model.hpp — Classic-only portable evidence and judge model; per-section clearance, 60/120/240 frame windows, Nerves, parser frame accounting, skillset/memory evidence, swag-route comparison, and readiness verdict.
- tests/test_dan_analyzer_model.cpp — tests Classic-only schema, idle exclusion, clearance plausibility, parser accounting, all frame-window rates, Nerves, easier/harder route behavior, incomplete route search, and unsupported hitboxes.
- docs/dan-analyzer-design.md — documents Classic-only responsibilities, parser contract, hitbox basis, route evidence, judge behavior, and deferred Dan calibration.
- README.md — updates the pipeline to distinguish Image Parser from Swag Route Detection and records Classic-only scope.
- .github/workflows/geodan-analyzer-tests.yml — unchanged; manual-only portable test workflow.

## Handoff / next action

- **Last updated:** 2026-11-25.
- **Completed:** Removed GameMode/Platformer from the analyzer model. Replaced the earlier geometry+simulation-per-segment requirement with parser evidence for recorded gameplay and separate swag-route evidence. Added validation for parser frame accounting, clearance ordering, 60/120/240 timing windows, Nerves, unsupported hitboxes, route completion, easier-route bypass fraction, and harder-route non-inflation. Updated docs and candidate README.
- **Remaining:** Add explicit consistency checks between section-level and level-level aggregates; settle exact Nerves semantics with the user; define route-search coverage/termination evidence; implement and test feature aggregation; only later calibrate Dan scoring on a held-out Classic dataset.
- **Exact stopping point:** Model and tests locally compiled from the exact content pushed to the candidate branch. No Geode build, parser integration, route simulation, or Dan calibration.
- **Next action:** Continue with consistency validation and feature aggregation, without adding a Dan formula until evidence semantics and calibration are ready.
- **Blockers / risks:** Image Parser and Swag Route Detection do not yet exist. Frame-window validity depends on actual recorder timestamp/frame precision. The simulator candidate is not validated. Nerves and bypass-fraction measurement definitions still need calibration.
- **Relevant commits:** bdd14552e31a00234fcdb0a9557122c9a2ed1f00 (Classic-only judge model); 485d10bb0779a28f62fade1ab010e161f8297c19 (tests); 8a062ab1573cef0ddcdde7282772aa990416ee24 (design); a5cff6e6a7f9a59b0bd68a87e27a448296676572 (README).

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
