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
