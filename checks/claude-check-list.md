# Claude Checklist

> **Owner:** Claude maintains this file. Codex reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read [`README.md`](./README.md) and [`codex-check-list.md`](./codex-check-list.md) as well as this file.

## Current status

- **Status:** Awaiting Claude's first entry.
- **Active task:** None recorded.
- **Current stage:** Not initialized.
- **Baseline at checklist creation:** `main` — `4464d24549cae1762094080f6b72f5e1d2b228a1` (README creation commit; verify current HEAD before relying on it).

## Project constraints

- Do not modify the archived `GeoDan-deprecated` repository.
- The old trajectory implementation is known to be broken. Do not consult, copy, port, or reuse it as a reference.
- Do not claim tests, builds, or in-game verification that were not actually performed.
- Read both agent checklists before project work and commit reviews.
- Update this file with concrete progress and a precise handoff before stopping.

## Active work

_No active Claude task recorded._

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
- **Completed:** No Claude work recorded yet.
- **Remaining:** Populate this section when work begins and before handing off.
- **Exact stopping point:** Not recorded.
- **Next action for Codex:** Read this file and the Codex checklist; coordinate before taking over overlapping work.
- **Blockers / risks:** None recorded.
- **Relevant commit:** `4464d24549cae1762094080f6b72f5e1d2b228a1` (baseline at creation; verify current HEAD).

## Session log

_Add newest entries first. Include date/time and relevant commit SHA._

### 2026-11-25 — combined Claude progress branch (`workflow-claude/combined-progress`)
- Built from `workflow-claude/recorder-live-from-codex` (`860be16`), then merged `workflow-claude/trajectory-from-codex` (`6a8741b`) and `workflow-claude/image-parser-from-codex` (`04bf467`).
- Conflicts resolved: `tools/src/overlay.cpp` (both trajectory and recorder includes kept; a single `onQuit` stops the recorder, then cleans up trajectory); `tools/mod.json` (kept level-recorder and portrait settings; trajectory description from Codex's branch; valid JSON checked); `checks/codex-check-list.md` (kept the trajectory-branch version unchanged; Codex's image-parser log entries are NOT included here, Codex should reconcile).
- Portable tests run separately with g++ -std=c++20 -Wall -Wextra -Werror: PASS test_hitbox_model, test_image_parser, test_recorder_geometry, test_recorder_model, test_recorder_png_encoder, test_recorder_session, test_recorder_worker, test_tool_registry, test_trajectory_model.
- Geode type-check (headers only, SDK noise filtered): overlay.cpp, main.cpp, recorder_live.cpp, trajectory.cpp clean.
- Not verified: Geode build of this exact branch; in-game behaviour of any feature; the trajectory's own PlayLayer/PlayerObject hooks in combination with the recorder.
- Known open items carried over: portrait-preview-test settings-match checker failure; dan-analyzer excluded.
- Not promoted. `main` and `~/main-workflow` not changed.
