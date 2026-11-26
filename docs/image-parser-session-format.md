# Recorder session format v1 — Image Parser handoff

**Status:** Candidate contract on `workflow-codex/level-recorder`. The writer and schema are experimental; no parser implementation or compatibility test exists yet.

## Directory layout

```text
<session>/
├── manifest.json
├── frames.csv
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

This v1 contract describes the current portable session sink only. The actual PNG encoder, background worker lifecycle, parser implementation, and end-to-end compatibility test are still pending. Changes to the manifest shape require a schema-version increment rather than silently reinterpreting v1 fields.

## Schema v2: MP4 storage

Sessions recorded with `FrameStorage::Mp4` use `schemas/recorder-session-v2.schema.json`. The PNG format above is unchanged and remains the default.

```text
<session>/
├── manifest.json
├── frames.csv
└── video.mp4
```

- `schema_version` is `2`, and the manifest carries `frame_storage: "mp4"`, `video_file: "video.mp4"` and `frames_directory: null`. No `frames/` directory is created.
- `video.mp4` is H.264 (`yuv420p`), constant-rate at `target_fps`, with one video frame per written CSV row in CSV order. The CSV holds the real timing; the video's timestamps are nominal.
- The CSV contract is identical: one row per written frame, same columns, same sequence rules.
- `complete` is written only after the encoder process exits with status 0. A failed or interrupted encode is `incomplete` or `error`, and `video.mp4` must not be used.

Consistency checks for v2, replacing checks 5 and 6 above:
- Decoded frame count of `video.mp4` equals the CSV row count and `frames_written`.
- The video's width and height equal the manifest's `width` and `height`.
- Gaps in CSV `sequence` are preserved as metadata only. The encoded video contains no synthesized frames for them.

Android does not use this format yet. It needs an in-process encoder, and that work is separate from this change.
