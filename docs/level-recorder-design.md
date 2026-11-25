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
- `frames.csv`: sequence, presentation timestamp in nanoseconds, delta from the immediately prior captured frame (which may have been dropped), missed intervals, and writer result. The portable writer currently emits `written` rows only; queue drops are counted in the manifest. For contiguous persisted sequences, delta equals the difference between persisted timestamps. Across a sequence gap, delta remains the interval since the immediately preceding captured frame and can be smaller than the timestamp difference between persisted rows.
- Optional geometry sidecars may be added later, but they must use the same sequence number as the corresponding image.

The image parser should consume a completed manifest, not scan an arbitrary folder while the recorder is still writing. A session is marked complete only after the writer drains and all files are closed. Interrupted sessions must be marked incomplete and retain enough metadata to diagnose the stop.

## Per-frame hitbox metadata

Every successfully written PNG also has a matching row group in `hitboxes.csv`. The worker writes the PNG, `frames.csv` entry, and geometry rows using the same original sequence number. The geometry stream is worker-owned and flushed with the frame metadata; a geometry write failure marks the session as an error rather than publishing a complete session with missing evidence.

The portable contract is in `core/recorder_geometry.hpp`; the session writer persists it from `core/recorder_session.hpp`, and `RecorderWorker` transports the snapshot in each queued frame. The game-facing adapter is `tools/src/recorder_geometry.hpp`. It maps object-layer coordinates through Cocos world space and the portrait capture transform into capture-pixel coordinates (top-left origin, y increasing downward).

The first adapter deliberately uses conservative axis-aligned envelopes:
- Player: the inner `getObjectRect(0.3f, 0.3f)` box already used by the diagnostic trail.
- Hazards: the existing hitbox overlay's circle/oriented-box/rectangle outline, reduced to a capture-space AABB.
- Lethal slope edges and unclassified interactive objects: mark the frame `unsupported` rather than silently omitting uncertain collision geometry.
- Decorative objects, disabled groups, and `m_isNoTouch` objects are omitted.
- Fully off-capture hazards are omitted; partially visible bounds are clipped to the capture rectangle.

These envelopes are conservative for rotated/circular shapes and are not proven to be the exact engine collision shapes. The player's inner box is also a diagnostic proxy until checked against the engine's collision routines. Do not treat this metadata as validated physics evidence yet. The adapter currently exists as a callable snapshot helper; no live frame-capture producer calls it because GPU readback and gameplay recorder lifecycle are still not integrated.

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
5. Reusable pixel-buffer pool and background PNG encoder/writer — portable encoder and worker added; test execution and runtime integration remain.
6. Portable session manifest and CSV writer — worker-owned sink implemented; synthetic tests added, execution still required.
7. Recorder worker lifecycle, queue drain/join and error propagation — portable worker added; gameplay start/stop controls remain.
8. Validate parser handoff against decoder-verified PNGs, then implement capability-gated GPU readback and profile at 60 FPS on target hardware.

## Explicit non-claims

No 60 FPS result, Geode build, image output, parser compatibility, or in-game behavior is claimed until measured. A successful pure-C++ model test does not establish GPU capture performance.

## Portable session writer checkpoint

`core/recorder_session.hpp` adds a Geode-free `SessionWriter` for the parser-facing directory contract. It is owned and called by the future encoder/writer worker (never the render thread) and accepts already-encoded PNG bytes; it deliberately does not implement PNG compression or GPU readback.

A new session directory contains a manifest initially marked `recording`, then finalized as `complete`, `incomplete`, or `error`; sequence-named PNG files under `frames/`; and `frames.csv` with sequence, monotonic timestamp, delta, missed cadence intervals and write result.

Frame files are written to temporary paths and renamed into place. The manifest is staged through a temporary file. Existing session directories are never overwritten. Sequence gaps remain visible. Duplicate/out-of-order sequences, non-monotonic timestamps, inconsistent deltas and payloads without the PNG signature are rejected. Queue drops and total capture-cadence misses are supplied as aggregate counters after the worker drains the producer queue, so they include frames that never reached disk.

The writer is deliberately synchronous and single-owner: it is a worker-side sink, not yet a thread manager. The future recorder lifecycle must close the producer queue, drain accepted frames, call `finalize`, then join the worker. If destroyed while active, it attempts to leave an incomplete manifest. Process crashes or power loss can still interrupt filesystem operations; this is not a transactional filesystem.



## PNG encoder and background worker checkpoint

Added `core/recorder_png_encoder.hpp`, a portable RGBA8-to-PNG encoder. It writes standard PNG signature/IHDR/IDAT/IEND chunks, computes PNG CRC-32 and zlib Adler-32 checksums, and uses valid DEFLATE stored blocks. It has no external compression-library dependency and performs no Cocos/OpenGL calls. The trade-off is intentionally low CPU complexity rather than a small file: stored DEFLATE blocks do not compress image data, so session size and disk throughput will be substantially higher than with a tuned compressor. Replacing it with a faster compression-level PNG backend is a later measured optimization, not a reason to block the first end-to-end test.

Added `core/recorder_worker.hpp`. The worker owns `SessionWriter` on its background thread, waits on the bounded queue, encodes RGBA leases, writes frames in queue order, aggregates missed-cadence and queue-drop counts, drains accepted frames after close, and joins before reporting completion. Queued pixel leases are released on success and on error while the worker drains remaining queue items. The caller must stop producing before calling `stop()`; the queue is then closed, drained, and the session is finalized. The pool and queue must outlive the worker and every queued lease.

Added synthetic PNG encoder and worker tests and included them in the existing manual-only portable test workflow. They have not been compiled or run in this environment. The PNG test currently checks basic structure/signature/dimensions; a real decoder validation should be added before treating the encoder as production-ready.

## GPU readback remains gated

The GPU readback backend is deliberately not wired yet. The existing portrait preview has not been built or visually checked, so the target's orientation, exact pixel dimensions, and content are not yet confirmed. More importantly, the repository targets Windows, Android, macOS, and iOS; PBO/map/fence entry points and extensions differ across those graphics backends. A guessed cross-platform GL call path would risk breaking builds or silently falling back to a synchronous stall. The next readback step is to validate the preview on the user's device/build, inspect the actual GL/GLES version and extensions, then add a capability-gated asynchronous readback backend with a measured non-blocking fallback. Do not use `CCRenderTexture::newCCImage` as the presumed 60 FPS path.

The new encoder and worker are portable components only. They are not yet connected to `PlayLayer`, the render target, or GPU readback; no recording controls or 60 FPS result are claimed.
