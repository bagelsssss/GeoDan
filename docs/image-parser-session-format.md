# Recorder session format v1 — Image Parser handoff

**Status:** Experimental Recorder Session v1 contract on `workflow-codex/level-recorder`. A portable writer now emits the geometry sidecar, and the separate Image Parser candidate can read it. No real-session compatibility or in-game validation has been performed.

## Directory layout

```text
<session>/
├── manifest.json
├── frames.csv
├── hitboxes.csv
└── frames/
    ├── frame_000000.png
    ├── frame_000001.png
    └── ...
```

The manifest shape is defined by `schemas/recorder-session-v1.schema.json`. The schema version is explicit and must be checked before parsing.

## Manifest semantics

- `status`: `recording` means the session may still be changing (or the process stopped unexpectedly); `complete` is a normally finalized session; `incomplete` is an intentionally stopped/aborted session; `error` indicates a writer or finalization failure.
- The parser should process `complete` sessions by default. It may offer `incomplete` sessions for explicit inspection, but must not treat `recording` or `error` as complete input.
- `width`, `height`, and `target_fps` describe the requested output. The parser must validate actual decoded PNG dimensions against width/height.
- `frames_written` is the number of successfully written PNGs, not the number of expected cadence slots.
- `missed_capture_intervals` is the aggregate number of absent capture-cadence slots reported by the capture timeline, including intervals associated with frames later dropped by the writer queue. `writer_queue_drops` counts captured frames rejected by the bounded writer queue. They are different failure modes and must not be combined.
- `start_unix_ns` and `end_unix_ns` are wall-clock Unix timestamps in nanoseconds. `end_unix_ns` is null while recording.
- Frame timestamps in CSV are monotonic nanoseconds relative to the recording timeline; they are not Unix timestamps and should be used for timing analysis.
- `first_sequence` and `last_sequence` are the actual first and last successfully written capture sequence numbers. They may be null for an empty session. They are not inferred from the frame count.
- `stop_reason` is diagnostic text, not a parser control token. Treat it as untrusted display text.

## CSV contract

UTF-8-compatible, comma-separated text with one header row and one row per successfully written PNG:

```csv
sequence,timestamp_ns,delta_ns,missed_intervals,write_status
0,0,0,0,written
2,33333333,16666666,1,written
```

- `sequence`: original capture sequence number. Gaps are valid and must be preserved.
- `timestamp_ns`: monotonic timestamp from the session start.
- `delta_ns`: time since the previous captured frame; the first written frame has delta 0.
- `missed_intervals`: capture-cadence intervals missed before this captured frame.
- `write_status`: currently `written` for every row; failed writes do not get a CSV row.

## Hitbox geometry sidecar

The recorder writes `hitboxes.csv` with one frame sentinel row and zero or more entity rows for every persisted PNG. Its exact header is:

```csv
sequence,phase,geometry_status,entity_type,entity_id,left,top,right,bottom
```

- `sequence` is the same original capture sequence used by `frames.csv` and the PNG filename.
- `phase` is `start_idle`, `active_gameplay`, `paused`, `respawn`, `transition`, or `unknown`.
- `geometry_status` is `valid` only when the adapter could enumerate the supported player/hazard geometry for that frame; otherwise it is `unsupported`.
- Each frame has exactly one `entity_type=frame` sentinel with blank entity fields.
- Entity rows use `player` or `hazard`, a per-frame unique numeric ID, and finite capture-pixel bounds.
- Coordinates use the PNG coordinate system: origin at top-left, x rightward, y downward. Bounds are clipped to the capture rectangle; fully off-capture hazards are omitted.
- Unsupported frames may retain partial entity rows for diagnostics, but parsers must not treat them as complete geometry or calculate clearance from them.

The current game-facing adapter uses the same diagnostic geometry helpers as the live hitbox overlay. It records the player's inner `getObjectRect(0.3, 0.3)` box and hazard shape envelopes transformed into capture pixels. Rotated and circular hazard shapes are represented by conservative axis-aligned bounding envelopes; lethal slope edges and unclassified interactive objects mark the frame unsupported. These are provisional geometry proxies, not yet verified exact engine collision shapes. The player box and hazard envelopes must be compared against the game's actual collision routines before calling the data authoritative.

The adapter is not yet connected to a live frame-capture producer. The portable SessionWriter and worker now carry and persist geometry with each frame, but the full GPU capture/worker producer integration remains pending.

## Frame naming and consistency checks

Frame filenames are `frame_` + sequence padded with at least six decimal digits + `.png`. Padding is a minimum width, not a maximum; larger sequence values are not truncated.

A parser should verify:
1. Manifest schema version is supported and status is eligible for parsing.
2. CSV header matches exactly and every data row parses.
3. CSV row count equals `frames_written`.
4. Sequence numbers and timestamps are strictly increasing. If the first sequence is 0, its delta is zero; otherwise its delta is positive. For contiguous persisted sequences, delta equals the timestamp difference. Across sequence gaps, delta is positive and no greater than the persisted timestamp difference because intervening captures may have been dropped.
5. Every CSV sequence has exactly one matching PNG file; no missing file is silently skipped.
6. Every decoded PNG has the manifest dimensions.
7. The first/last CSV sequence agrees with manifest first/last sequence, or both are null for an empty session.

Do not synthesize frames to fill sequence gaps or force the output to 60 FPS. Real timestamps are authoritative; gaps and drops are evidence the analyzer may need.

## Publication rule

The recorder writes a manifest with status `recording` when a session starts. On clean shutdown it drains accepted work, closes files, and replaces the manifest with status `complete`. On intentional abort it writes `incomplete`; on writer failure it attempts to write `error`. A parser should not start consuming a session while its status is `recording`.

The current writer does not provide a filesystem transaction across all files. A crash can leave a `recording` manifest, temporary files, or a partially written session. The parser must reject or quarantine malformed/inconsistent sessions rather than guessing.

## Compatibility note

This v1 contract describes the portable session sink and its geometry sidecar. The PNG encoder and background worker exist as portable components, but GPU readback and the live PlayLayer producer are not connected. The Image Parser implementation exists on `workflow-codex/image-parser`; end-to-end compatibility and in-game validation remain pending. Changes to the manifest shape require a schema-version increment rather than silently reinterpreting v1 fields.
