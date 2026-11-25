# GeoDan Image Parser — first implementation contract

## Scope

This is a portable, Geode-free parser for Classic Geometry Dash Recorder Session v1. It validates the recording manifest, CSV timeline, PNG file presence/dimensions, and a companion `hitboxes.csv` sidecar containing per-frame hitbox metadata. It groups frames into chronological sections of at most ten persisted images, splitting early when the gameplay phase changes, and measures player-to-hazard clearance from the supplied bounds.

The parser does not infer collision geometry from decorative pixels. The Recorder now has a portable writer for `hitboxes.csv` and a game-facing geometry snapshot adapter, but that adapter is not connected to a live capture producer and its geometry proxies have not been validated in-game. End-to-end compatibility remains pending.

## Input files

The session must contain:

- `manifest.json`
- `frames.csv`
- `hitboxes.csv`
- `frames/frame_<sequence padded to at least six digits>.png`

The hitbox sidecar has this exact header:

```csv
sequence,phase,geometry_status,entity_type,entity_id,left,top,right,bottom
```

There is one `frame` sentinel row per persisted frame. It declares the phase and whether geometry enumeration for that frame is complete. Entity rows list the player's and collision-enabled hazard bounds intersecting the capture rectangle for that frame.

- `phase`: `start_idle`, `active_gameplay`, `paused`, `respawn`, `transition`, or `unknown`. Unknown is never treated as active gameplay.
- `geometry_status`: `valid` or `unsupported`.
- `entity_type`: `frame`, `player`, or `hazard`.
- `entity_id`: unsigned integer, unique per entity type within a frame; blank on a `frame` sentinel.
- `left`, `top`, `right`, `bottom`: finite capture-pixel coordinates, origin at top-left, x increasing rightward and y increasing downward. Rectangles must have positive width and height. Fields are blank on a `frame` sentinel.

The current Recorder adapter emits axis-aligned bounding envelopes from its diagnostic geometry helpers. These are conservative for rotated/circular shapes and are not yet proven to be the exact engine collision shapes. The player's inner box is also a diagnostic proxy until checked against the game's collision routines. Do not call this data authoritative physics geometry until validated.

For `valid` geometry, at least one player rectangle is required. Zero hazards means no clearance sample for that frame; it is not converted into zero clearance or an assumed safe measurement. If the Recorder cannot enumerate every relevant collidable hazard/player hitbox, it must mark the frame `unsupported`. Unsupported frame geometry is retained as an unclassified frame and is never treated as empty space.

## Validation

The parser rejects sessions with an unsupported manifest schema/status, malformed CSV, row-count mismatch, non-monotonic sequence/timestamps, inconsistent `delta_ns`, missing or unexpected geometry frames, missing PNGs, wrong PNG dimensions, invalid rectangles, duplicate entity IDs, conflicting per-frame geometry metadata, or missing frame sentinels.

PNG validation checks the PNG signature, required 13-byte IHDR header, and IHDR dimensions. It does not fully decompress or checksum image pixel data. Clearance measurements come from the hitbox sidecar, not image recognition; accuracy is bounded by the geometry quality and support status supplied by the Recorder.

Recorder sequence gaps, per-row missed intervals, manifest `missed_capture_intervals`, and `writer_queue_drops` remain distinct. `timestampGapCount` counts persisted-frame rows that indicate a discontinuity (sequence gap or missed intervals), not the number of missing frames. Original sequence numbers and monotonic timestamps are preserved; no frames are synthesized.

## Sections and clearance

Frames are consumed in CSV chronology. Sections contain at most ten persisted images and are split at phase boundaries so idle/pause/respawn/transition evidence cannot be blended with active gameplay. Section sample indexes refer to zero-based persisted-frame ordinals; original capture sequence numbers remain in frame metadata.

For each active frame with valid player and hazard geometry, the parser calculates the minimum Euclidean gap between any player rectangle and any hazard rectangle. Overlapping/touching rectangles have zero clearance. The minimum pairwise gap for that frame is one clearance sample, preventing a frame with many hazards from receiving extra statistical weight. Section and level reports include arithmetic mean, nearest-rank median, minimum, and nearest-rank 10th percentile in normalized pixels. Coordinates arrive in capture pixels and all distances are scaled uniformly to a 360-pixel reference width (360 / manifest width).

Frames with valid geometry but no hazard rectangle do not produce a clearance sample. This is missing measurement, not evidence of safety. Unsupported geometry increments the section's unsupported-hitbox count and the level's unclassified-frame count.

## Explicitly not calculated yet

- 60/120/240 FPS timing-window counts: 60 FPS images cannot prove sub-frame 120/240 FPS windows. A validated high-rate timing-event telemetry contract is required.
- Nerves: the Analyzer has no finalized Nerves formula. The parser leaves it Missing rather than inventing a score.
- Skillset tags: the current session does not include the required input/mode/event telemetry for reliable classification.
- Memory classification: not implemented.
- Analyzer schema adapter: output remains a parser-owned portable report; an adapter to the Analyzer schema-v3 model is a later integration step.

`parserComplete` means all persisted frames were structurally processed and had supported geometry. It does not mean all Analyzer-required evidence (timing windows, Nerves, skillsets) is available or that a Dan can be assigned.

## Recorder dependency

The Recorder candidate is on `workflow-codex/level-recorder`. It now writes a `hitboxes.csv` sidecar from geometry carried with each queued frame. The game-facing snapshot adapter exists but is not connected to a live frame-capture producer; GPU readback and the gameplay recorder lifecycle remain pending.