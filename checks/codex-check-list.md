# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read [`README.md`](./README.md) and [`claude-check-list.md`](./claude-check-list.md) as well as this file.

## Current status

- **Status:** Active — isolated Level Recorder candidate; not approved for promotion.
- **Active task:** Implement a 360 × 1080 portrait level recorder with black background, diagnostic overlays, parser-ready session output, and a 60 FPS performance target.
- **Current stage:** Portable recorder model and implementation contract added; game capture path not yet implemented.
- **Candidate branch:** `workflow-codex/level-recorder`
- **Branch base:** `main` at `5cfd2dc01f17761f77be4aee6970f8b1998b10f2`
- **Latest candidate commit at checklist update:** To be recorded after this update commit.
- **Local workspace:** `~/workflow-codex` has not been verified in this session. Remote GitHub branch is the candidate source.
- **Promotion status:** Experimental only. No approval to promote into `~/main-workflow`.
- **Main-workflow changed:** No.
- **Other candidate:** `workflow-codex/trajectory-rebuild` is separate and untouched by this recorder task.

## Project constraints

- Do not modify the archived `GeoDan-deprecated` repository.
- The old trajectory implementation is known to be broken. Do not consult, copy, port, or reuse it as a reference.
- Do not claim tests, builds, or in-game verification that were not actually performed.
- Read both agent checklists before project work and commit reviews.
- Update this file with concrete progress and a precise handoff before stopping.

## Active work

- **Status:** Active — isolated Level Recorder candidate; implementation is incomplete.
- **Task:** Build the new continuous level recorder, replacing screenshot-at-a-time behavior with a parser-oriented recording session.
- **User decisions:** Initial configurable capture resolution is 360 × 1080. The target is 60 FPS footage. Frames use a black background with enabled diagnostic tools composited into the image. Output is handed to the image parser.
- **Performance requirements:** No blocking disk I/O or image compression on the game/render thread; bounded queue; explicit timestamps and dropped-frame accounting; no fabricated duplicate frames.
- **Remote branch:** `workflow-codex/level-recorder`
- **Base SHA:** `5cfd2dc01f17761f77be4aee6970f8b1998b10f2`
- **Promotion status:** Experimental only. User has not approved promotion.
- **Main-workflow changed:** No.
- **Implementation so far:** Added portable config/timing/backpressure model, unit tests, recorder design contract, and manually triggered test workflow. No Cocos/GPU capture, image encoder, session writer, UI, or parser handoff has been implemented yet.
- **Trajectory candidate:** Separate branch `workflow-codex/trajectory-rebuild`; not merged or modified.

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

### 2026-11-25 — Level Recorder candidate kickoff and portable foundation
- Created isolated branch `workflow-codex/level-recorder` from `main` SHA `5cfd2dc01f17761f77be4aee6970f8b1998b10f2`.
- Read root README, shared checklist guide, both agent checklists, current main tree, tools CMake, tool registry, mod settings and gameplay overlay.
- Added `core/recorder_model.hpp`: configurable capture dimensions/FPS/queue capacity, RGBA byte-budget helpers, monotonic frame timestamps, missed-cadence accounting, and a bounded non-blocking producer queue.
- Added `tests/test_recorder_model.cpp` covering config, timestamps, missed intervals, bounded queue, drops, close and drain behavior. Test execution is not yet recorded.
- Added `docs/level-recorder-design.md` with capture/rendering, 60 FPS, queue, session format, lifecycle, failure and profiling contracts.
- Added `.github/workflows/geodan-recorder-model.yml`, triggered only by `workflow_dispatch`; no per-commit build/test trigger was added and no workflow was run.
- User-selected resolution: 360 × 1080. Target cadence: 60 FPS. No guarantee is claimed before profiling.
- No Geode capture code, Cocos render target, pixel readback, PNG encoder, session writer, recorder controls, or image-parser integration has been implemented.
- No build, unit test, Actions run, in-game test, or performance benchmark has been executed in this session.
- `~/main-workflow` and `main` were not changed. The trajectory candidate branch was not touched.
- Next: implement and validate capture target/camera framing, share overlay draw logic, then bounded pixel-buffer pool + background encoder/writer and parser session handoff. Profile GPU readback separately; do not claim 60 FPS until measured.

