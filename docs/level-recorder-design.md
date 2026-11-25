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

1. Capture at most once for each real rendered game frame.
2. Use a monotonic high-resolution clock for timestamps.
3. Store a contiguous capture sequence number and the actual presentation timestamp for every image.
4. Record missed 60 Hz intervals and writer-queue drops separately.
5. Never block the gameplay/render thread on compression, filesystem writes, or waiting for a free queue slot.
6. Use a bounded queue and a fixed-size reusable pixel-buffer pool. If no buffer is available, skip that capture and increment a counter.
7. Compress and write images on a worker thread. All Cocos/OpenGL calls and GPU readback stay on the render thread unless a platform-specific asynchronous readback path is proven safe.
8. Do not create duplicate images to pad a recording to 60 FPS. The manifest must preserve real timing and gaps.

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

The exact readback path must be selected at runtime from verified graphics capabilities. Do not assume desktop OpenGL PBO support exists on every Android/iOS backend. If a platform has no viable asynchronous readback path, report that limitation rather than claiming 60 FPS capture is guaranteed.

## Session output contract

A recording session is a directory containing:
- `manifest.json`: schema version, capture dimensions, target FPS, actual captured count, missed cadence intervals, queue drops, start/end timestamps, and completion/error status.
- `frames/frame_000000.png`, etc.: one image for each successfully encoded capture. Sequence numbers must not be renumbered to hide gaps.
- `frames.csv`: sequence, presentation timestamp in nanoseconds, delta from prior capture, missed intervals, and writer result.
- Optional geometry sidecars may be added later, but they must use the same sequence number as the corresponding image.

The image parser should consume a completed manifest, not scan an arbitrary folder while the recorder is still writing. A session is marked complete only after the writer drains and all files are closed. Interrupted sessions must be marked incomplete and retain enough metadata to diagnose the stop.

## Failure and lifecycle handling

- Start only in active gameplay after the capture target and output session are successfully initialized.
- Stop cleanly on user request, level exit, restart, scene transition, or unrecoverable capture/write error.
- On stop, stop accepting frames, close the queue, drain accepted frames, join the writer, then finalize the manifest.
- Never detach a writer thread that can outlive its recorder or mod.
- If the queue fills, drop the incoming frame without waiting and record the drop.
- If GPU readback itself stalls, the CPU queue cannot solve it. Profile it separately and only add platform-specific asynchronous readback where the supported backend provides a correct implementation.

## Implementation sequence

1. Portable timing, queue and configuration model — **started**.
2. Unit tests for cadence, timestamp monotonicity, queue bounds and drops — **added; execution still required**.
3. Cocos capture target and portrait camera transform.
4. Refactor existing overlay draw code to render into the recorder target without changing normal gameplay appearance.
5. Reusable pixel-buffer pool and background image writer.
6. Session manifest and CSV writer, finalized only after worker drain.
7. Recorder start/stop controls and lifecycle cleanup.
8. Profile at 60 FPS on the target device and report frame time, capture time, encode throughput, queue depth, cadence misses and drops.

## Explicit non-claims

No 60 FPS result, Geode build, image output, parser compatibility, or in-game behavior is claimed until measured. A successful pure-C++ model test does not establish GPU capture performance.
