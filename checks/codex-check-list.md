# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read [`README.md`](./README.md) and [`claude-check-list.md`](./claude-check-list.md) as well as this file.

## Current status

- **Status:** Active — isolated Level Recorder candidate; not approved for promotion.
- **Active task:** Implement a 360 × 1080 portrait level recorder with black background, diagnostic overlays, parser-ready session output, and a 60 FPS performance target.
- **Current stage:** Temporary disabled-by-default portrait preview test integrated; needs local build/in-game visual validation. Portable worker-owned session writer and synthetic tests added; test execution still required. No GPU readback or PNG encoder.
- **Candidate branch:** `workflow-codex/level-recorder`
- **Branch base:** `main` at `5cfd2dc01f17761f77be4aee6970f8b1998b10f2`
- **Latest verified candidate HEAD before this checklist update:** `4bb9aefb556115f9a35a80ddf24975b9a3e0a1c5`.
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
- **Implementation so far:** Added portable config/timing/backpressure model, anchored fixed-rate capture gate, reusable preallocated RGBA buffer pool, bounded non-blocking queue and model tests. Shared hitbox/trail renderer is used by live PlayLayer. Added portrait target and disabled-by-default preview harness. Added `core/recorder_session.hpp`, a worker-owned portable sink for encoded PNG frames, versioned manifest, ordered frame files and CSV metadata, plus synthetic session tests. No GPU readback, actual PNG encoder, worker-thread lifecycle, controls or parser integration yet.
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

- **Last updated:** 2026-11-25.
- **Completed:** Isolated recorder branch created; portable capture config/timing/backpressure model, bounded queue, reusable pixel buffer pool, tests, design contract, and manual-only test workflow committed.
- **Remaining:** User must build and visually validate the portrait preview. Independently, run the portable model and session tests; implement PNG encoding and worker lifecycle that drains the bounded queue into SessionWriter; define/verify parser handoff; later validate GPU readback and profile on target hardware.
- **Exact stopping point:** Temporary preview integration committed but not locally built or visually tested. Portable SessionWriter and tests are present but not executed; no pixel encoder or worker lifecycle exists.
- **Next action:** Continue Codex work in `~/workflow-codex` only after verifying that workspace and its branch/HEAD; otherwise continue through this isolated remote branch without claiming local workspace access.
- **Blockers / risks:** Synchronous `CCRenderTexture::newCCImage` is unsuitable as an assumed 60 FPS hot path; cross-platform asynchronous GPU readback support is unverified. No test/build/Actions/in-game run has occurred.
- **Promotion:** Not approved; `~/main-workflow` unchanged.
- **Relevant commit before checklist update:** `4e258bfe560501d658fe9cce476f58e242fea53b`.

## Session log

### 2026-11-25 — Level Recorder candidate kickoff and portable foundation
- Created isolated branch `workflow-codex/level-recorder` from `main` SHA `5cfd2dc01f17761f77be4aee6970f8b1998b10f2`.
- Read root README, shared checklist guide, both agent checklists, current main tree, tools CMake, tool registry, mod settings and gameplay overlay.
- Added `core/recorder_model.hpp`: configurable capture dimensions/FPS/queue capacity, RGBA byte-budget helpers, fixed-rate capture gate, monotonic frame timestamps, missed-cadence accounting, bounded non-blocking producer queue, and a preallocated reusable pixel-buffer pool.
- Added `tests/test_recorder_model.cpp` covering config, 60 Hz pacing, timestamps, missed intervals, bounded queue, drops, close/drain behavior, and buffer-pool reuse. Test execution is not yet recorded.
- Added `docs/level-recorder-design.md` with capture/rendering, 60 FPS, queue, session format, lifecycle, failure and profiling contracts.
- Added `.github/workflows/geodan-recorder-model.yml`, triggered only by `workflow_dispatch`; no per-commit build/test trigger was added and no workflow was run.
- User-selected resolution: 360 × 1080. Target cadence: 60 FPS. No guarantee is claimed before profiling.
- No Geode capture code, Cocos render target, pixel readback, PNG encoder, session writer, recorder controls, or image-parser integration has been implemented.
- No build, unit test, Actions run, in-game test, or performance benchmark has been executed in this session.
- `~/main-workflow` and `main` were not changed. The trajectory candidate branch was not touched.
- Next: implement and validate capture target/camera framing, share overlay draw logic, then bounded pixel-buffer pool + background encoder/writer and parser session handoff. Profile GPU readback separately; do not claim 60 FPS until measured.


### 2026-11-25 — recorder performance refinement
- Added a fixed-rate gate anchored to the initial 60 Hz timeline. It skips extra renders when the game renders faster than target and accumulates small delays so sustained sub-60 cadence is eventually counted as missed intervals.
- Added a preallocated reusable RGBA pixel-buffer pool with non-blocking leases. The capture thread can skip instead of allocating or waiting when all buffers are in use.
- Expanded tests for 120 Hz render pacing, accumulated cadence drift, and buffer reuse. These tests remain unexecuted.
- Inspected the Geode 2.2081 `CCRenderTexture` binding and Geode `CCImage` implementation. The inspected `newCCImage` implementation performs synchronous `glReadPixels` and allocates full-frame buffers; raw `CCImage::initWithImageData` copies supplied pixels. The design therefore explicitly avoids treating those convenience APIs as an optimized per-frame path.
- Latest source commit before this checklist update: `4e258bfe560501d658fe9cce476f58e242fea53b`.


### 2026-11-25 — shared overlay renderer extraction
- Verified current candidate HEAD was `e4411d17aace2cdc76683f741cdd759005a8c656`; branch remains isolated and is 18 commits ahead of main with no behind commits.
- Read the shared checklist guide, both agent checklists, candidate README, recorder design contract, tool menu, build configuration, mod settings, and full gameplay overlay source.
- Added `tools/src/overlay_renderer.hpp` to centralize hitbox geometry/color rendering and trail drawing. Updated `tools/src/overlay.cpp` so the live overlay calls the shared routines; this is intended to let a future recorder target render the same diagnostics without a second implementation.
- Updated the recorder design contract to describe the extraction and its limits.
- No Cocos render target, camera framing, GPU readback, image encoding, session writer, controls, or parser handoff was added.
- No tests, Geode build, in-game visual check, or performance measurement was run. The new Cocos-facing header therefore remains uncompiled/unverified.
- Changed files: `tools/src/overlay_renderer.hpp` (added), `tools/src/overlay.cpp`, `docs/level-recorder-design.md`, `checks/codex-check-list.md`.
- Next: validate the extraction in the user's local Geode build; then investigate a supported offscreen render-target/camera path and platform-specific readback before writing capture code. Keep all work on this candidate branch; do not promote.


### 2026-11-25 — renderer include audit
- Follow-up source inspection found the extracted header uses `std::abs`; added its direct `<cmath>` dependency rather than relying on transitive Geode includes.
- This is a source-level correction only. No compiler/build result is claimed.


### 2026-11-25 — portrait offscreen target primitive
- Verified candidate HEAD before edits: `2cfd0c5d55d3c7e7367875f6875065f4705591ec`; current main baseline remains `5cfd2dc01f17761f77be4aee6970f8b1998b10f2`.
- Inspected the pinned Geode 2.2081 Cocos headers for `CCRenderTexture`, `CCNode::transformAncestors/visit`, `CCCamera`, `CCDirector`, and Kazmath matrix stack APIs. Confirmed render texture dimensions are specified in points, `beginWithClear` exists, `CCNode::transformAncestors()` is public, and `kmGLPushMatrix/PopMatrix/Translatef/Scalef` are available.
- Added `tools/src/portrait_capture_target.hpp`: owns a retained 360×1080-pixel RGBA8888 render texture (accounts for content scale factor and checks actual texture dimensions), clears to opaque black, and renders a supplied world subtree with an explicit world-space focus and scale while applying its live ancestor transforms. It does not change the live camera or reparent nodes.
- Added a helper to obtain a node's Cocos world position.
- Updated design contract and this checklist. No gameplay hook/integration was added because the exact target subtree and desired portrait crop must be verified visually; the likely object-layer target is still a proposal, not a confirmed fact.
- No build, test, in-game capture, pixel readback, PNG encoding, session writer, or performance measurement was run. The new header is uncompiled and unverified.
- Changed files: `tools/src/portrait_capture_target.hpp` (added), `docs/level-recorder-design.md`, `checks/codex-check-list.md`.
- Next: local Geode build and visual test to confirm transform stack correctness, actual output dimensions, and whether `m_objectLayer` contains the desired level/player/overlay content without HUD. Only after that integrate into PlayLayer and choose the crop scale.

### 2026-11-25 — temporary in-game portrait preview harness
- Verified candidate HEAD before edit: c48dbb0660b835fbf1bb6c3a68d2b119dcce6cde; no local workspace access or build is claimed.
- Added disabled-by-default Geode setting `portrait-preview-test`.
- When enabled, PlayLayer::postUpdate initializes the 360×1080 render target once, renders m_objectLayer centered on player 1 using current game-window height as the initial crop reference, and displays the render texture as a small top-right preview.
- The preview is outside m_objectLayer, so it is not recursively captured. It hides when the setting is disabled. No pixel readback, files, encoder, or session writer are involved.
- The test renders an additional pass every gameplay update while enabled; this is deliberately temporary and may affect performance. Disable it after a short visual check.
- Added documentation describing the test and its limitations.
- No build, runtime, visual, or performance test has been executed here. User must build/install the candidate and inspect the preview in-game.
- Changed: tools/src/overlay.cpp, tools/mod.json, docs/level-recorder-design.md, checks/codex-check-list.md.
- Next: build candidate, enable Portrait capture preview (TEST) in Geode mod settings, enter a level, and report visual results/build errors. Do not proceed to GPU readback until this passes.
- Follow-up source review added the missing `capture` namespace alias used by the temporary preview hook; this is a source-level correction, not a build result.

### 2026-11-25 — portable recording session writer
- Verified branch HEAD before work: `0f197adc547ced7dabb2b47757bd3a6653970c52`; main baseline remains `5cfd2dc01f17761f77be4aee6970f8b1998b10f2`.
- User approved continuing with work that does not require the portrait preview test.
- Added `core/recorder_session.hpp`, a portable worker-owned sink for already-encoded PNG bytes. It creates a new session directory, writes a recording manifest, sequence-named frame files and CSV metadata, and finalizes complete/incomplete/error state.
- Frame files and manifest use temporary files followed by rename. Existing session directories are not overwritten. Sequence gaps remain visible; duplicate/out-of-order sequences, non-monotonic timestamps, inconsistent deltas and non-PNG signatures are rejected.
- Queue drops are separately recorded in the manifest; cadence misses are summed from written frame stamps. Writer has no internal thread: future lifecycle must own it on the background worker, drain the queue, finalize, and join.
- Added `tests/test_recorder_session.cpp` using synthetic PNG-signature payloads. These tests exercise filesystem layout/metadata, finalization, abort, duplicate rejection and no-overwrite behavior. They have NOT been compiled or run here; the synthetic payload does not validate a real PNG encoder.
- Updated the design contract and this checklist. No image parser integration, PNG encoder, GPU readback, recorder controls, or worker thread was added.
- Changed: `core/recorder_session.hpp` (new), `tests/test_recorder_session.cpp` (new), `docs/level-recorder-design.md`, `checks/codex-check-list.md`.
- No local workspace, build, test, in-game run, or benchmark is claimed. `main` and `~/main-workflow` remain unchanged; candidate is experimental.
- Next: run both portable recorder tests in the user's environment; then implement a worker lifecycle and connect an encoder interface. Do not start real capture until the portrait preview test passes.
- Source audit follow-up: manifest now records the actual first frame sequence (not an assumed zero); synthetic test uses sequence 10 then 12 to exercise a gap.

- Updated the existing `workflow_dispatch`-only portable test workflow to compile and run the session-writer test alongside the recorder model test. No automatic per-commit trigger was added, and the workflow has not been dispatched.
