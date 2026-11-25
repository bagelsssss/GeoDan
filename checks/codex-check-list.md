# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read [`README.md`](./README.md) and [`claude-check-list.md`](./claude-check-list.md) as well as this file.

## Current status

- **Status:** Active — isolated Level Recorder candidate; not approved for promotion.
- **Active task:** Implement a 360 × 1080 portrait level recorder with black background, diagnostic overlays, parser-ready session output, and a 60 FPS performance target.
- **Current stage:** Portable hitbox sidecar writer, per-frame geometry model, worker transport, and PlayLayer geometry snapshot adapter added. Live capture producer is still not connected; GPU readback, preview validation, and current sidecar tests remain pending.
- **Candidate branch:** `workflow-codex/level-recorder`
- **Branch base:** `main` at `5cfd2dc01f17761f77be4aee6970f8b1998b10f2`
- **Latest verified candidate HEAD before this checklist update:** `4a47d2bdfc6acd1ba6a6c665e74dee4d543ab3bb`.
- **Local workspace:** `~/workflow-codex` has not been verified in this session. Remote GitHub branch is the candidate source.
- **Promotion status:** Experimental only. No approval to promote into `~/main-workflow`.
- **Main-workflow changed:** No.
- **Other candidates:** `workflow-codex/trajectory-rebuild` is separate and untouched. Claude's review branch `workflow-claude/level-recorder-from-codex` is at `47f3cb5d41a047611b5b2dfdd2b545d9ef7651e3`, based on our earlier `92e6fb0`; it contains a two-line Geode-prelude fix only and is not merged.

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
- **Implementation so far:** Added portable capture timing/backpressure, portrait target, shared overlay renderer, PNG encoder, worker and session writer. New `core/recorder_geometry.hpp` defines capture-pixel bounds, frame phase/status, coordinate mapping and CSV serialization. `SessionWriter` now creates `hitboxes.csv` and writes one sentinel plus player/hazard rows for every persisted PNG. `QueuedFrame` transports geometry through the worker. `tools/src/recorder_geometry.hpp` snapshots player/hazard bounds from `PlayLayer`, maps them through the portrait transform, and marks unclassified/unsupported geometry explicitly. Rotated/circular hazards use conservative AABBs; lethal slope edges are unsupported. This is a provisional diagnostic proxy, not verified exact engine collision geometry. The snapshot helper is not yet called by a live capture producer.
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
- **Exact stopping point:** Portable PNG encoder and background worker are committed with synthetic tests and manual workflow steps, but no tests/build have been executed here. Worker is not connected to PlayLayer. GPU readback remains intentionally unimplemented until preview and backend validation.
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

- Added the explicit v1 Image Parser handoff document and JSON Schema. It defines manifest status handling, Unix-vs-monotonic timestamp semantics, CSV columns, frame naming, sequence gaps, parser consistency checks, and incomplete-session behavior. No parser code or compatibility run is claimed.

- Corrected frame-delta validation for writer-queue drops: a persisted sequence gap means `delta_ns` is measured from the immediately prior captured frame, not the prior persisted frame. Contiguous rows must match timestamp difference; gap rows retain their real capture delta. Writer I/O failures now make a best-effort `error` manifest update.

- Aggregate capture misses are now supplied independently from persisted CSV rows, alongside queue-drop totals after worker queue drain. This preserves cadence misses associated with frames that were subsequently dropped before disk write.


### 2026-11-25 — portable PNG encoder and background worker
- Verified the recorder branch files and the session-writer contract at commit `b904b683a2a84f506dbc96f22470a964662fe429`; all work remains isolated on `workflow-codex/level-recorder`.
- Added `core/recorder_png_encoder.hpp`: a dependency-free RGBA8 PNG encoder using valid zlib DEFLATE stored blocks, PNG CRC-32 and Adler-32 checksums. This avoids a third-party library but prioritizes predictable low CPU complexity over compression ratio; output size/disk throughput must be measured.
- Added `core/recorder_worker.hpp`: a background worker that owns SessionWriter, consumes the bounded queue, encodes RGBA pixel leases, writes frames in order, aggregates queue drops/cadence misses, drains accepted frames on stop, finalizes, and joins. The queue and pixel pool must outlive the worker and producer must stop before worker stop.
- Added `tests/test_recorder_png_encoder.cpp` and `tests/test_recorder_worker.cpp`; extended the manual-only workflow to compile/run both tests. No test, build, or workflow dispatch was run here.
- Updated `docs/level-recorder-design.md` with the encoder/worker contracts and explicit GPU-readback gate.
- GPU readback is not implemented yet: preview orientation/content is unvalidated, and the project targets Windows, Android, macOS and iOS with different GL/GLES extension availability. Do not guess PBO/map/fence APIs or substitute synchronous `newCCImage` as a 60 FPS path. Next: run portable tests and validate the portrait preview; then inspect the active graphics backend/extensions and implement a capability-gated asynchronous readback path.
- Exact changed files: `core/recorder_png_encoder.hpp`, `core/recorder_worker.hpp`, `tests/test_recorder_png_encoder.cpp`, `tests/test_recorder_worker.cpp`, `.github/workflows/geodan-recorder-model.yml`, `docs/level-recorder-design.md`, `checks/codex-check-list.md`.
- No local workspace, Geode build, test execution, Actions run, in-game test, GPU readback, performance benchmark, or promotion is claimed. `main` and `~/main-workflow` remain unchanged.



### 2026-11-25 — Recorder hitbox sidecar and Claude cross-review handoff
- Re-read the shared workflow guide and both agent checklists before this task. Current main's Claude checklist records the user's workflow decision that Claude reviews Codex's candidate, then builds/reviews on a Claude-owned branch; no changes were made to Claude's checklist.
- Verified current Codex Recorder branch before checklist update: `workflow-codex/level-recorder`, HEAD `4a47d2bdfc6acd1ba6a6c665e74dee4d543ab3bb`. It is based on main `5cfd2dc01f17761f77be4aee6970f8b1998b10f2`; current main has advanced by 9 documentation commits (README and Claude checklist only), with no conflicting source-code changes. Candidate remains experimental.
- Added `core/recorder_geometry.hpp`: portable frame geometry, capture transform (world/Cocos to top-left capture-pixel coordinates), clipping/intersection, validation, and exact hitboxes.csv row serialization. Valid frames require player evidence; unsupported frames can preserve uncertainty.
- Updated `core/recorder_session.hpp` to create and write `hitboxes.csv` in lockstep with PNG and `frames.csv` output. Metadata failures make the session error/incomplete rather than silently publishing missing geometry.
- Updated `core/recorder_worker.hpp` so each queued frame carries its matching geometry snapshot to the worker-owned SessionWriter.
- Added `tools/src/recorder_geometry.hpp`: PlayLayer snapshot adapter using the existing diagnostic hitbox geometry helpers and portrait capture transform. It skips decorations, disabled groups and no-touch objects; marks unclassified interactive objects and lethal slope edges unsupported. It uses conservative AABB envelopes for rotated/circular hazards and the player's existing inner diagnostic box. This is not proven exact engine collision geometry.
- Added/updated tests: `tests/test_recorder_geometry.cpp`, `tests/test_recorder_session.cpp`, and `tests/test_recorder_worker.cpp`. Updated the manual-only recorder workflow to include the geometry test. These new sidecar/session/worker tests have NOT been run in this session; no Actions run is claimed.
- Claude's checklist records a review branch `workflow-claude/level-recorder-from-codex` at `47f3cb5d41a047611b5b2dfdd2b545d9ef7651e3`, based on our earlier `92e6fb0`. Claude ran the portable suite successfully on that earlier revision and type-checked `overlay.cpp` (no SDK build or in-game test). Claude found `overlay_renderer.hpp` needed `using namespace geode::prelude;`; I inspected Claude's exact two-line diff and applied that small source fix to this Codex branch with provenance. Claude's review branch has not incorporated the newer sidecar work.
- Updated the separate Image Parser candidate `workflow-codex/image-parser` (latest `a0426901f160943c097cce2ea64657d0d3b4fc0c`) to accept `unknown` phase rows, validate per-frame metadata consistency even for unknown phases, and document the provisional AABB geometry limitations.
- Important limitation: `snapshotGeometry` is not yet called by a live frame-capture producer. The current Recorder still lacks the GPU readback/PlayLayer enqueue lifecycle, so no real session is produced yet. Portrait preview remains unvalidated.
- Potential contract decision before freezing: the v1 manifest fields did not change, but the session directory now includes a required parser sidecar. Consider whether to keep the manifest schema at v1 with an added sidecar file or explicitly bump the session contract to v2. No version bump was made.
- Tests/build record: Claude's earlier tests at base revision `92e6fb0` passed for hitbox model, recorder geometry/model/PNG/session/worker, and tool registry; `overlay.cpp` type-check had only known header noise. This does NOT validate the newer sidecar writer/worker changes. No current test, Geode SDK build, Actions dispatch, in-game capture, GPU readback, or performance test was run here.
- Exact files changed on this branch: `core/recorder_geometry.hpp`, `core/recorder_session.hpp`, `core/recorder_worker.hpp`, `tools/src/recorder_geometry.hpp`, `tools/src/overlay_renderer.hpp`, `tests/test_recorder_geometry.cpp`, `tests/test_recorder_session.cpp`, `tests/test_recorder_worker.cpp`, `.github/workflows/geodan-recorder-model.yml`, `docs/image-parser-session-format.md`, `docs/level-recorder-design.md`, this checklist.
- No `main`, `~/main-workflow`, or Claude-owned branch was changed. Local `~/workflow-codex` was not accessed; all edits were on the isolated remote Codex branch.
- **Claude handoff request:** Review latest `workflow-codex/level-recorder` HEAD `4a47d2b`, especially the new geometry adapter, CSV sidecar writer and worker transport. Please run the updated portable tests on a Claude-owned branch based on this latest revision, report exact outcomes and geometry/API concerns in Claude's checklist, and do not edit the Codex branch.
- **Next Codex action:** after Claude's review, resolve any issues, connect the geometry snapshot to the actual capture producer once GPU readback/recorder lifecycle is ready, and validate the coordinate mapping against a real portrait frame.



### 2026-11-25 — follow-up: Claude parser integration evidence
- Re-read Claude's updated main checklist after the prior handoff. Claude also reviewed `workflow-codex/image-parser` at `a042690`, fixed malformed string escaping in the parser width-normalization test on `workflow-claude/image-parser-from-codex` (`04bf467`), and reports the parser tests passed with assertions enabled.
- Claude reports a scratch integration check where a session written by its Recorder branch's `SessionWriter` with a real 360×1080 PNG was accepted by `parseSession`. Verified that `core/recorder_session.hpp` and `core/recorder_worker.hpp` blob SHAs on Claude's Recorder branch match this Codex branch, so the scratch check exercised the same sidecar-writing code.
- This is portable scratch integration evidence, not a committed cross-branch test, a live PlayLayer recording, a Geode SDK build, or an in-game capture.
- The newer Codex geometry model/adapter and sidecar tests have changed since Claude's review base. Claude handoff remains: review current Recorder HEAD and rerun the full portable suite on a Claude-owned branch; report exact output and geometry/API concerns in Claude's checklist.
- Claude's parser string-escape fix has also been applied to `workflow-codex/image-parser`; no Claude-owned branch was changed.



### 2026-11-25 — align with Claude's live-integration scope
- Re-read the latest `main/checks/claude-check-list.md` after updating this checklist. Claude independently confirms the Recorder's live pipeline is not connected: `SessionWriter`, `RecorderWorker`, `snapshotGeometry`, and portrait target exist, but no PlayLayer producer joins them and GPU readback is not implemented.
- Claude also found the `level-recorder` tool remains marked Planned in `core/tool_registry.hpp`; it is not enabled in the Tools menu.
- Claude's open user decision is whether to attempt a synchronous `CCRenderTexture::newCCImage` readback as a first functional fallback (with measured frame drops and no 60 FPS guarantee) or wait for the user's local Geode build/preview validation before selecting a readback path. No readback implementation or tool-registry change was made here.
- Current next step is blocked on the user's choice of readback strategy. Keep the tool disabled until the capture path is connected and validated.


## 2026-11-25 — Codex full self-review (Tools foundation + Recorder library)

**Review status:** Portable test suite passed. Geode Windows build and in-game behaviour are not verified. Candidate remains isolated and is not approved for promotion.

- Reviewed branch: workflow-codex/level-recorder
- Implementation/test head: 340d62365b00428ad79cdaeddc5da7ec057c357e
- Branch base: main snapshot 5cfd2dc01f17761f77be4aee6970f8b1998b10f2
- Commits since branch base: 74. Full ordered commit list and diffs: https://github.com/bagelsssss/GeoDan/compare/5cfd2dc01f17761f77be4aee6970f8b1998b10f2...340d62365b00428ad79cdaeddc5da7ec057c357e
- Main was read at feb19b3e68e9c1996e673248a7cd006cd630c7a1; this candidate is based on the older recorded base above.
- No changes to main, ~/main-workflow, Claude-owned branches, or GeoDan-deprecated.

### Files changed relative to the branch base

- .github/workflows/geodan-recorder-model.yml — manual portable Recorder model/session/encoder/worker/geometry checks.
- .github/workflows/geodan-codex-review-tests.yml — one-time, path-filtered self-review workflow; each test has its own compile-and-run step.
- checks/codex-check-list.md — Codex implementation log and this review.
- core/recorder_geometry.hpp — portable frame geometry, capture transform, clipping, validity and CSV sidecar serialization.
- core/recorder_model.hpp — capture configuration, 60 Hz timeline, missed-interval accounting, bounded queue and reusable pixel-buffer leases.
- core/recorder_png_encoder.hpp — dependency-free RGBA8 PNG writer using stored DEFLATE blocks, CRC-32 and Adler-32.
- core/recorder_session.hpp — worker-owned session manifest, frames CSV, hitboxes CSV, PNG persistence and finalize/abort lifecycle.
- core/recorder_worker.hpp — background queue consumer, PNG encoding, sidecar persistence, queue drain and finalization.
- core/tool_registry.hpp — review correction: trajectory is Planned because this branch does not include its implementation; Level Recorder remains Planned because live capture is absent.
- docs/image-parser-session-format.md — Recorder Session v1 and geometry sidecar contract.
- docs/level-recorder-design.md — capture, performance, output and limitations.
- schemas/recorder-session-v1.schema.json — manifest schema.
- tests/test_recorder_geometry.cpp — capture-pixel transforms, clipping and sidecar serialization tests.
- tests/test_recorder_model.cpp — capture timing, queue and buffer-pool tests.
- tests/test_recorder_png_encoder.cpp — PNG structure tests.
- tests/test_recorder_session.cpp — session persistence, sequence and lifecycle tests.
- tests/test_recorder_worker.cpp — worker queue drain, PNG output and finalization tests.
- tests/test_tool_registry.cpp — registry invariants and three-Available-tool count.
- tools/mod.json — false-by-default hitboxes, unclassified, trail and diagnostic portrait preview settings; removed the unavailable trajectory setting.
- tools/src/main.cpp — pause-menu popup; trajectory is no longer presented as Available in this candidate.
- tools/src/overlay.cpp — PlayLayer diagnostic overlay and temporary portrait preview; no live Recorder producer.
- tools/src/overlay_renderer.hpp — shared hitbox and trail rendering routines.
- tools/src/portrait_capture_target.hpp — retained 360×1080 RGBA8888 offscreen render target with explicit crop transform.
- tools/src/recorder_geometry.hpp — PlayLayer-to-portable geometry snapshot adapter.

### Geode 5.11.2 / GD 2.2081 Windows binding audit

Checked the actual declaring class blocks in bindings/2.2081/GeometryDash.bro from geode-sdk/bindings revision d14587bf6c08b9114b5605ec6a3c42cc16b02de6 (blob 1d194f26246c69adf609a136d0d8056a4702a739):

| Modify class | Hooked method | Windows binding |
|---|---|---|
| PauseLayer | customSetup() | win 0x37c870, not inline |
| PlayLayer | postUpdate(float) | win 0x3b4ba0, not inline |
| PlayLayer | resetLevel() | win 0x3b8eb0, not inline |

No Windows-inline hook found in these classes. The tool popup and overlay are still Geode-facing code and have not been compiled against the Windows SDK in this review.

### Test record

All tests below were run separately on GitHub Actions Ubuntu 26.04.1 at implementation/test head 340d62365b00428ad79cdaeddc5da7ec057c357e. The job completed successfully. -I./core -I. is the checkout-root equivalent of -I<dir>/core -I<dir>. Each step's successful conclusion means exit code 0.

| Test | Exact command | Exit | Stdout |
|---|---|---:|---|
| test_hitbox_model.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_hitbox_model.cpp -o /tmp/geodan-hitbox_model-review && /tmp/geodan-hitbox_model-review | 0 | hitbox model + trail: all checks passed |
| test_recorder_geometry.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_recorder_geometry.cpp -o /tmp/geodan-recorder_geometry-review && /tmp/geodan-recorder_geometry-review | 0 | recorder geometry: all checks passed |
| test_recorder_model.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_recorder_model.cpp -o /tmp/geodan-recorder_model-review && /tmp/geodan-recorder_model-review | 0 | recorder model: all checks passed |
| test_recorder_png_encoder.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_recorder_png_encoder.cpp -o /tmp/geodan-recorder_png_encoder-review && /tmp/geodan-recorder_png_encoder-review | 0 | recorder PNG encoder: basic structure checks passed |
| test_recorder_session.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_recorder_session.cpp -o /tmp/geodan-recorder_session-review && /tmp/geodan-recorder_session-review | 0 | recorder session: all checks passed |
| test_recorder_worker.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_recorder_worker.cpp -o /tmp/geodan-recorder_worker-review && /tmp/geodan-recorder_worker-review | 0 | recorder worker: queue drain, PNG output and finalization checks passed |
| test_tool_registry.cpp | g++ -std=c++20 -Wall -Wextra -Werror -I./core -I. tests/test_tool_registry.cpp -o /tmp/geodan-tool_registry-review && /tmp/geodan-tool_registry-review | 0 | tool registry: all checks passed |

Workflow: https://github.com/bagelsssss/GeoDan/actions/runs/45558618691

### Settings and registry

- Available tools in this candidate: hitboxes, unclassified, hitbox-trail.
- trajectory, level-recorder, and macro-bot are Planned; their settings are not exposed as enabled toggles.
- portrait-preview-test is deliberately in mod.json but not core/tool_registry.hpp: it is a temporary diagnostic render-preview setting, not a selectable tool. This is semantically correct, but the current strict check_commit.py settings-match check may still flag the extra setting unless it is explicitly allowlisted or the test setting is removed after the preview is retired.
- The preview setting is false by default. All user-facing tool settings are false by default.

### What works, what does not, and what remains unverified

- Portable PASS: hitbox/trail data model, Recorder timing and queue model, geometry transforms and sidecar serialization, PNG encoder structure, session writer, worker, and registry tests.
- Implemented but not runtime-verified: hitbox outlines, unclassified outlines, trail display, Tools popup, portrait offscreen render target and PlayLayer geometry snapshot adapter.
- Explicitly not implemented: the live Recorder path. No PlayLayer producer reads pixels, queues frames, starts/stops RecorderWorker, or connects the capture target to SessionWriter. This branch cannot record a playable level yet. Do not copy the separate Claude live Recorder path into this branch.
- Geometry limitation: the sidecar uses provisional AABBs for rotated/circular hazards and a diagnostic inner player rectangle; those are not yet validated as exact engine collision geometry. Lethal slope edges are marked unsupported.
- PNG limitation: the encoder uses uncompressed/stored DEFLATE blocks. PNG structure test passes; compression ratio, disk throughput, and 60 FPS viability have not been measured.
- UI limitation: no dedicated unit test validates popup placement, button-row selection or menu interactions.
- Geode Windows build: NOT RUN / not verified.
- In-game test of this branch: NOT RUN in this review. No visual proof of hitboxes, trail, popup, portrait target or geometry coordinate alignment is claimed.
- Performance profile: NOT RUN. No 60 FPS guarantee is made.

### Review commits made

- b0205c273982b625eac17be80260ee898bde678b — mark trajectory Planned until its implementation is present.
- 333de931919530bd01577b82b39a48960b6d84be — remove unavailable trajectory setting.
- 1fdb99c5b173e07f8b03031e16efd634df2f10c3 — move unavailable trajectory to the Planned popup section.
- 2f597b5987bc78bd910b070897e000bcb7c90bcc — align registry test with three implemented tools.
- 340d62365b00428ad79cdaeddc5da7ec057c357e — add isolated one-time test workflow and run the suite.

Next action: Geode Windows build and in-game validation must be performed by the user on this candidate before anyone wires live capture or promotes it.
