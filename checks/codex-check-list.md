# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read README.md and claude-check-list.md as well as this file.

## Current status

- **Status:** Active — initial portable Dan Analyzer evidence model and readiness gate added; no scoring formula.
- **Active task:** Build a measurement-first Dan Analyzer that consumes evidence from geometry parsing and the Simulator.
- **Current stage:** Evidence schema/model foundation; feature aggregation and calibration remain future stages.
- **Current branch:** workflow-codex/dan-analyzer.
- **Latest verified HEAD before this checklist update:** 291ebb4cf2c4881828095d48287268ccabb57e1a.

## Project constraints

- Do not modify the archived GeoDan-deprecated repository.
- Do not consult, copy, port, or reuse the old broken trajectory implementation.
- Do not claim tests, builds, or in-game verification that were not actually performed.
- Read both agent checklists before project work and commit reviews.
- Keep this candidate isolated; do not modify main-workflow or promote without explicit user approval.
- Keep GitHub Actions manual-only and use them sparingly.
- Prioritize measurement accuracy over tuning dan labels.
- Do not calibrate alpha–kappa in the initial analyzer pass.
- Unknown evidence is not zero; unsupported interactions are not safe/empty space.

## Active work

- **Status:** Active — isolated analyzer candidate, not approved for promotion.
- **Workspace:** ~/workflow-codex (local checkout not verified; remote branch is the source edited in this session).
- **Branch:** workflow-codex/dan-analyzer, based on main at 5cfd2dc01f17761f77be4aee6970f8b1998b10f2.
- **Promotion state:** Experimental; not approved for ~/main-workflow.
- **Main workflow changed:** No.

## Decisions and findings

- **User decision:** Tools and Level Recorder may be marked mostly implemented but not tested. README records them as candidate implementations, not promoted/validated features.
- **User decision:** Bot search policy remains single.
- **User decision:** Memory threshold policy remains both. Exact semantics and numeric threshold are not guessed; visual obstruction and simulation unclear-path evidence are kept separate.
- **User decision:** Final Dan measurement policy remains both. Exact combination semantics are not guessed beyond recording geometry and simulation evidence independently.
- **User decision:** Classic and platformer are distinct supported game modes.
- **User decision:** Preserve the requested skillset taxonomy: Circles, Clicksync, Fast-Paced, Timings, Chokepoints, Unbalanced, Learny, Memory, High CPS, Gimmicky, Flow, Slow-Paced, Bossfight, Mirror, Nerve Control, Cube, Ship, Ball, UFO, Wave, Robot, Spider, Old Swing, New Swing, Duals, Overall.
- **Verified repository finding:** Main had no Dan Analyzer implementation at branch creation; the analyzer is being started as a new portable core model.
- **Verified repository finding:** The trajectory candidate branch contains a first hold/release simulator, but it is not built or validated in-game.
- **Physics evidence review:** Uploaded geodan-log(2).txt records a rotated-slope run with noclip=unverified. It shows a flat-floor grounded event around frame 312, slope/direction hook calls for object 1743 around frames 552–576, support_match=0, and an airborne transition around frame 577. This is diagnostic evidence, not a validated physics oracle.
- **Repository consistency issue:** The trajectory branch's checklist names 9bfee125b1a0c8a413522fe0025ae0a195c78de8 as latest, but GitHub's branch listing returned bd4498f51d43ff24f5c0bebb9cbbb83f2801b5d1. Treat the actual remote ref as source of truth; reconcile checklist later.
- **Design proposal:** Calibration eligibility currently requires validated geometry and simulation evidence per segment, with no unsupported interactions. This is a conservative readiness gate, not a finalized dan formula.

## Test and build record

| Date | Command / test | Result | Notes |
|---|---|---|---|
| 2026-11-25 | Portable model test using local source-equivalent copy: g++ -std=c++20 -Wall -Wextra -Werror | Passed | Output: Dan analyzer evidence model: all checks passed. Covers classic and platformer segments, idle sample exclusion, evidence provenance, unsupported interactions, numeric bounds and skillset validation. The test/header were reconstructed source-equivalently from the committed files; exact remote files were not compiled directly. |
| 2026-11-25 | GitHub Actions analyzer workflow | Not run | Workflow is workflow_dispatch only. |
| 2026-11-25 | Geode build / in-game tests | Not run | Model is Geode-free; no analyzer UI or game integration exists. |

## Files changed

- core/dan_analyzer_model.hpp — Geode-free evidence types, sample-indexed mode/phase segments, per-segment geometry/simulation measurements, skillset evidence, separate memory signals, conservative readiness gate, and 1st–10th Dan tier labels.
- tests/test_dan_analyzer_model.cpp — tests missing evidence, classic/platformer mode handling, startup-idle exclusion, validated source requirements, unsupported interactions, invalid numeric values, skillset validation, and tier bounds.
- docs/dan-analyzer-design.md — measurement-first architecture, current pipeline status, physics-test limitations, and next stages.
- README.md — marks Tools and Level Recorder as candidate mostly implemented but untested; records parser, simulator, and analyzer status.
- .github/workflows/geodan-analyzer-tests.yml — manual-only portable test workflow.

## Handoff / next action

- **Last updated:** 2026-11-25.
- **Completed:** Started the isolated analyzer branch from main. Added a portable evidence model, tests, architecture notes, pipeline status update and a manual-only test workflow.
- **Remaining:** Verify exact committed source with the test compiler; define parser-to-analyzer and simulator-to-analyzer input contracts; design segment aggregation; define skillset evidence aggregation; assemble calibration dataset; only then implement 1st–10th dan assignment.
- **Exact stopping point:** Analyzer candidate branch workflow-codex/dan-analyzer. Latest code/docs commit before this checklist update: e9eef4179a660470bc655facbbd5ce6dc1f72bd4.
- **Next action:** Continue with a versioned evidence contract and segment-level feature aggregation. Do not implement a dan score formula yet.
- **Blockers / risks:** Physics evidence includes a run with noclip unverified and support_match=0 on the slope hook; trajectory candidate not built/in-game validated; capture/parser not end-to-end validated; calibration dataset/held-out set not ready.
- **Relevant commits:** 2821cde01b627a092f81ae24446a4d6ed2c9c4fc (model); 23569e093b7d40eab204a4d97ce26b54d541978a (tests); 01c93c0897538cb503b8185cb102fb3bd9f62c1c (design); a625d7082df7d3f480beaefedb244d33536669e8 (README); fea78c0d9132d82f3e3bd1c378db935156f99887 (unsupported evidence); 5a167fcb3d26de42b9ae37e0325322c67362abbd (unsupported evidence test); c71268a4d3ec5934deb0d95d14708539096b73f8 (manual test workflow); 291ebb4cf2c4881828095d48287268ccabb57e1a (design clarification); 48429eaa4c89b41dba53d438c533e53b37fcb02a (sample-index ranges and metric bounds); b349c9c3f993aa0893e09a3dba171b6d9d6699ef (range tests); 6d304f564dc2b5b9abd7c3b67520b8e8cc31d70c (classic/platformer test); a23bec0b73ca5377967a21afb2848a438feed630 (active-vs-idle phase); 97261e82fa369faee561622c0a41d0cd7ebf6935 (idle exclusion test); e9eef4179a660470bc655facbbd5ce6dc1f72bd4 (design notes).

## Session log

### 2026-11-25 — Dan Analyzer evidence model foundation
- Read the shared workflow guide and both agent checklists on main; checked remote branch list and candidate files.
- Confirmed main has no analyzer implementation. Created workflow-codex/dan-analyzer from main, keeping the analyzer work isolated.
- Reviewed the trajectory candidate and uploaded rotated-slope test log. The log header says noclip=unverified; slope hook records have support_match=0 before an airborne transition. Did not treat this as validated physics.
- Added the portable evidence model and tests. The model stores source/state/sample count for every metric; unknown is distinct from zero; unsupported evidence remains explicit. Segment ranges use sample indices rather than world X, and explicit phases keep startup idle/pause/respawn/transition samples out of difficulty calculations.
- Added all 26 requested skillset labels and classic/platformer mode fields. Added 1st–10th Dan tier names only; no tier mapping or score formula.
- Added conservative readiness gate; its per-segment geometry+simulation requirement is documented as a proposal, not a final scoring rule.
- Added manual-only test workflow and updated README pipeline statuses. Tools README wording now clearly says the candidate is isolated and not tested together. Tools and Level Recorder are recorded as candidate mostly implemented, untested and not promoted.
- A local source-equivalent C++ test copy compiled with g++ -std=c++20 -Wall -Wextra -Werror and passed after adding classic/platformer and idle-phase coverage. Exact remote source compilation and GitHub Actions were not run.
- No Geode code, gameplay hook, simulator physics, main-workflow, or main branch changes.
