# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read README.md and claude-check-list.md as well as this file.

## Current status

- **Status:** Active — Classic-only Dan Analyzer now includes section aggregation and parser/route consistency verification; no calibrated Dan formula.
- **Active task:** Prepare the portable judge for Image Parser integration.
- **Current stage:** Section aggregation, level-summary consistency, and swag-route adjustment evidence.
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
- **Verified previous branch head:** f15de523c78930f6970e18b8320ec21c2a55706a. Compare against workflow-codex/dan-analyzer reported 8 commits ahead, 0 behind before this checklist update.

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
- **Remaining:** Run the exact branch test suite. Add integration with the future Image Parser and Swag Route Detection. Define Nerves semantics and route-search coverage/termination evidence with the user. Calibrate the eventual Dan formula only on a held-out Classic dataset.
- **Exact stopping point:** Code, tests, and documentation committed to the isolated remote branch. The new test suite was authored but NOT compiled or run in this session. No GitHub Actions run, Geode build, parser integration, route simulation, or Dan calibration.
- **Next action:** Run `g++ -std=c++20 -Wall -Wextra -Werror -Icore tests/test_dan_analyzer_model.cpp -o /tmp/geodan-dan-analyzer-tests && /tmp/geodan-dan-analyzer-tests` in the Codex checkout or manually dispatch the existing portable test workflow; fix any failures before extending the model.
- **Blockers / risks:** Image Parser and Swag Route Detection do not yet exist. Frame-window validity depends on actual recorder timestamp/frame precision. The simulator candidate is not validated. Nerves and bypass-fraction measurement definitions still need calibration. Whole-level frame median/p10 cannot be exactly reconstructed from section summary percentiles; the model deliberately reports section-level distributions separately.
- **Relevant commits:** 8582cf6e751f87db2be3e9c646dcbcbd5a4b7f88 (aggregation); e72f0f84236b9ea9e9e696346d5b5a7650d0ae33 (validation hardening); d620f39adcd7de5f9555a579408463c9cb973343 (tests); 40b4bbe06fa620c272ea3d46aa9e3e75c2d17a65 (design docs); 109139b73fe743d61a6b3f18836251a4ce5fa13f (additional verification tests).

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
