# GeoDan Level Recorder — implementation contract

**Candidate branch:** `workflow-codex/level-recorder`  
**Status:** Initial portable scheduling/backpressure model implemented; game capture and encoder integration are not yet implemented or validated.

## User requirements

- Replace the former screenshot-style workflow with a continuous level recorder.
- Output tall, narrow portrait frames; initial configurable size is **360 × 1080**.
- Render the level over a solid black background.
- Include the enabled GeoDan diagnostic tools in every captured frame: hitboxes, hitbox trails, trajectory, and future overlays.
- Hand the resulting recording session to the image parser.
- Target 60 captured frames per second, with performance prioritized.

## 60 FPS contract

60 FPS is a performance target, not a guarantee independent of hardware or game frame rate. The recorder must never invent frames or conceal missed captures.

1. Pace captures to the configured 60 Hz timeline: at 120 Hz rendering, capture every second render; at 60 Hz, capture each render.
2. Use a monotonic high-resolution clock for timestamps.
3. Store a contiguous capture sequence number and the actual presentation timestamp for every image.
4. Record missed 60 Hz intervals and writer-queue drops separately.
5. Never block the gameplay/render thread on compression, filesystem writes, or waiting for a free queue slot.
6. Use a bounded queue and a fixed-size reusable pixel-buffer pool. If no buffer is available, skip that capture and increment a counter.
7. Compress and write images on a worker thread. All Cocos/OpenGL calls and GPU readback stay on the render thread unless a platform-specific asynchronous readback path is proven safe.
8. Do not create duplicate images to pad a recording to 60 FPS. The manifest must preserve real timing and gaps. If the game cannot supply 60 distinct states per second, the recorder must report a degraded/incomplete cadence rather than inventing footage.

## Performance budget

At RGBA8, 360 × 1080 is 1,555,200 bytes per frame. At 60 FPS the raw pixel throughput is 93,312,000 bytes/s (about 89 MiB/s), before compression and copies.

The initial queue capacity of three frames bounds queued raw pixels to 4,665,600 bytes (about 4.45 MiB), excluding the active render/readback buffer and encoder workspace. Queue capacity should remain configurable but bounded.

Avoid per-frame heap churn:
- Allocate the render target once per recording session.
- Preallocate the pixel-buffer pool and queue slots.
- Reuse metadata and encoder scratch storage where the encoder permits.
- Avoid full-frame copies; transfer ownership of a completed buffer to the writer.
- Do not run PNG compression on the game thread.

## Rendering/composition contract

The recorder owns a dedicated 360 × 1080 offscreen target. Each capture clears it to opaque black, then draws a camera-framed gameplay region and the diagnostic overlays into the same target. The target must not include unrelated pause-menu UI or OS UI.

The capture must preserve the same geometry, colors, and overlay visibility as the game frame. Existing overlay code should be refactored so its drawing logic can be invoked both for normal gameplay and for the recorder target; do not maintain a second, subtly different implementation of hitbox/trail drawing.

The camera/crop transform must be explicit and stable for a session. Do not guess its world-space framing from device resolution. The initial capture should be validated visually before treating the frame layout as a parser contract.

## Readback API hazard discovered during API review

The Geode 2.2081 Cocos bindings expose `CCRenderTexture::newCCImage`, but the inspected inline implementation reads pixels with `glReadPixels` synchronously and allocates two full-frame byte arrays before constructing a `CCImage`. That is an unsuitable default hot path for a 60 FPS recorder because it introduces a GPU/CPU synchronization point and repeated full-frame allocations.

Do not call `CCRenderTexture::newCCImage` or `CCRenderTexture::saveToFile` once per frame as the final optimized design. Prefer:
- a reusable render target;
- a preallocated ring of pixel buffers;
- asynchronous GPU readback (pixel-pack buffers / supported extension) where the active graphics backend actually supports it;
- a non-blocking fallback that measures the readback cost and records skipped captures rather than stalling gameplay;
- background PNG encoding and file writes after CPU ownership of a completed buffer is established.

The inspected 2.2081 inline implementation is here: [CCRenderTexture.cpp](https://github.com/geode-sdk/bindings/blob/975068022f6164c483907274909ad2f700c6d646/bindings/2.2081/inline/CCRenderTexture.cpp). The common `CCImage::initWithImageData(kFmtRawData)` implementation also copies the supplied pixel bytes into a second allocation: [CCImageCommon_cpp.h](https://github.com/geode-sdk/geode/blob/8fe3b2db0656462483ab89c05a4f469233616b1c/loader/include/Geode/cocos/platform/CCImageCommon_cpp.h). Avoid adding that extra copy to every frame if a direct worker-side PNG encoder can consume the pooled RGBA buffer safely.

The exact readback path must be selected at runtime from verified graphics capabilities. Do not assume desktop OpenGL PBO support exists on every Android/iOS backend. If a platform has no viable asynchronous readback path, report that limitation rather than claiming 60 FPS capture is guaranteed.

## Session output contract

A recording session is a directory containing:
- `manifest.json`: schema version, capture dimensions, target FPS, actual captured count, missed cadence intervals, queue drops, start/end timestamps, and completion/error status.
- `frames/frame_000000.png`, etc.: one image for each successfully encoded capture. Sequence numbers must not be renumbered to hide gaps.
- `frames.csv`: sequence, presentation timestamp in nanoseconds, delta from prior capture, missed intervals, and writer result. The portable writer currently emits `written` rows only; queue drops are counted in the manifest.
- Optional geometry sidecars may be added later, but they must use the same sequence number as the corresponding image.

The image parser should consume a completed manifest, not scan an arbitrary folder while the recorder is still writing. A session is marked complete only after the writer drains and all files are closed. Interrupted sessions must be marked incomplete and retain enough metadata to diagnose the stop.

## Failure and lifecycle handling

- Start only in active gameplay after the capture target and output session are successfully initialized.
- Stop cleanly on user request, level exit, restart, scene transition, or unrecoverable capture/write error.
- On stop, stop accepting frames, close the queue, drain accepted frames, join the writer, then finalize the manifest.
- Never detach a writer thread that can outlive its recorder or mod.
- If the queue fills, drop the incoming frame without waiting and record the drop.
- If GPU readback itself stalls, the CPU queue cannot solve it. Profile it separately and only add platform-specific asynchronous readback where the supported backend provides a correct implementation.

## Temporary in-game portrait preview test

A disabled-by-default Geode setting, `portrait-preview-test`, initializes the 360×1080 target and continuously renders `m_objectLayer` focused on player 1. The render texture is displayed as a small portrait preview in the top-right of the gameplay layer. This allows checking whether the object layer includes the player, level objects, and diagnostic overlays while excluding pause UI. It renders every gameplay update while enabled and is a temporary visual test only: it does not read pixels back, write files, or guarantee 60 FPS. Disable it after testing because it adds a second render pass.

The initial crop scales the current game-window height to the target height and centers on player 1. This is a test heuristic, not a finalized camera/framing policy. Render-texture vertical orientation and transform correctness must be inspected in-game.

## Portrait offscreen target checkpoint

Added `tools/src/portrait_capture_target.hpp`, a small Cocos render-target owner for exact pixel dimensions. It converts requested output pixels to Cocos points using the active content scale factor, verifies the resulting texture dimensions, retains/releases the render texture, clears each frame to opaque black, and visits a caller-supplied world subtree under an explicit focus point and scale. It uses `CCNode::transformAncestors()` so the subtree retains its live scene-graph transforms, while applying the portrait framing outside those transforms. The live game camera and scene graph are not reparented or modified.

The caller must pass a world subtree that contains the desired level/player/overlay content but excludes HUD and pause UI, plus a focus point in Cocos world coordinates. The class deliberately does not choose the scale or crop policy. A likely first integration is the gameplay object layer focused on the active player, but the actual 2.2081 layer composition and visual result still need in-game verification. This target does not read pixels back, encode frames, or start a recording session. No build or in-game test has been run.

## Shared overlay renderer checkpoint

The current gameplay overlay has been split so the hitbox geometry and trail drawing routines live in `tools/src/overlay_renderer.hpp`. The live `PlayLayer` overlay calls the shared hitbox renderer and shared trail drawing routine. A future offscreen recorder target can call those same functions rather than duplicating parser-visible colors, geometry, and trail styling. This is a rendering-code extraction only: no offscreen target, camera transform, or capture path is implemented yet. The extraction has not been built or visually validated.

## Implementation sequence

1. Portable timing, queue and configuration model — **started**.
2. Unit tests for cadence, timestamp monotonicity, queue bounds and drops — **added; execution still required**.
3. Cocos capture target and portrait camera transform — target primitive added; layer selection and crop/scale validation remain.
4. Refactor existing overlay draw code to render into the recorder target without changing normal gameplay appearance.
5. Reusable pixel-buffer pool and background PNG encoder/writer — buffer pool added; encoder/thread integration remains.
6. Portable session manifest and CSV writer — worker-owned sink implemented; synthetic tests added, execution still required.
7. Recorder worker lifecycle, queue drain/join, start/stop controls and error propagation.
8. Validate parser handoff against real encoded frames, then profile at 60 FPS on target hardware.

## Explicit non-claims

No 60 FPS result, Geode build, image output, parser compatibility, or in-game behavior is claimed until measured. A successful pure-C++ model test does not establish GPU capture performance.
\n## Portable session writer checkpoint\n\n`core/recorder_session.hpp` adds a Geode-free `SessionWriter` for the parser-facing directory contract. It is owned and called by the future encoder/writer worker (never the render thread) and accepts already-encoded PNG bytes; it deliberately does not implement PNG compression or GPU readback.\n\nA new session directory contains a manifest initially marked `recording`, then finalized as `complete`, `incomplete`, or `error`; sequence-named PNG files under `frames/`; and `frames.csv` with sequence, monotonic timestamp, delta, missed cadence intervals and write result.\n\nFrame files are written to temporary paths and renamed into place. The manifest is staged through a temporary file. Existing session directories are never overwritten. Sequence gaps remain visible. Duplicate/out-of-order sequences, non-monotonic timestamps, inconsistent deltas and payloads without the PNG signature are rejected. Queue drops are counted separately from cadence misses.\n\nThe writer is deliberately synchronous and single-owner: it is a worker-side sink, not yet a thread manager. The future recorder lifecycle must close the producer queue, drain accepted frames, call `finalize`, then join the worker. If destroyed while active, it attempts to leave an incomplete manifest. Process crashes or power loss can still interrupt filesystem operations; this is not a transactional filesystem.\n\n