# GeoDan Image Parser — first implementation contract

## Scope

This is a portable, Geode-free parser for Classic Geometry Dash Recorder Session v1. It validates the recording manifest, CSV timeline, PNG file presence/dimensions, and a companion \`hitboxes.csv\` sidecar containing authoritative collision geometry. It groups frames into chronological sections of at most ten persisted images, splitting early when the gameplay phase changes, and measures player-to-hazard clearance from physical rectangles.

The parser does not infer collision geometry from decorative pixels. The Recorder's current v1 output does not yet write \`hitboxes.csv\`; therefore this parser candidate is not end-to-end compatible with existing recorder sessions until the Recorder produces the sidecar described below.

## Input files

The session must contain the Recorder Session v1 files:

- \`manifest.json\`
- \`frames.csv\`
- \`frames/frame_<sequence padded to at least six digits>.png\`

It must also contain \`hitboxes.csv\` with this exact header:

\`\`\`csv
sequence,phase,geometry_status,entity_type,entity_id,left,top,right,bottom
\`\`\`

There is one \`frame\` sentinel row per persisted frame. It declares the phase and whether geometry enumeration for that frame is complete. Entity rows then list the player's and all visible, collision-enabled hazard hitboxes for that frame.

- \`phase\`: \`start_idle\`, \`active_gameplay\`, \`paused\`, \`respawn\`, or \`transition\`.
- \`geometry_status\`: \`valid\` or \`unsupported\`.
- \`entity_type\`: \`frame\`, \`player\`, or \`hazard\`.
- \`entity_id\`: unsigned integer, unique per entity type within a frame; blank on a \`frame\` sentinel.
- \`left\`, \`top\`, \`right\`, \`bottom\`: finite coordinates in the PNG capture-pixel coordinate system, origin at top-left, x increasing rightward and y increasing downward. Rectangles must have positive width and height. Fields are blank on a \`frame\` sentinel.

For \`valid\` geometry, at least one player rectangle is required. Zero hazards means no clearance sample for that frame; it is not converted into zero clearance or an assumed safe measurement. If the Recorder cannot enumerate every relevant collidable hazard/player hitbox, it must mark the frame \`unsupported\`. Unsupported frame geometry is retained as an unclassified frame and is never treated as empty space.

## Validation

The parser rejects sessions with an unsupported manifest schema/status, malformed CSV, row-count mismatch, non-monotonic sequence/timestamps, inconsistent \`delta_ns\`, missing or unexpected geometry frames, missing PNGs, wrong PNG dimensions, invalid rectangles, duplicate entity IDs, or missing frame sentinels.

PNG validation currently checks the PNG signature, the required 13-byte IHDR header, and IHDR dimensions. It does not fully decompress or checksum image pixel data. Actual collision measurements come from the hitbox sidecar, not image recognition.

Recorder sequence gaps, per-row missed intervals, manifest \`missed_capture_intervals\`, and \`writer_queue_drops\` remain distinct. \`timestampGapCount\` counts persisted-frame rows that indicate a discontinuity (sequence gap or missed intervals), not the number of missing frames. Original sequence numbers and monotonic timestamps are preserved; no frames are synthesized.

## Sections and clearance

Frames are consumed in CSV chronology. Sections contain at most ten persisted images and are split at phase boundaries so idle/pause/respawn/transition evidence cannot be blended with active gameplay. Section sample indexes refer to zero-based persisted-frame ordinals; original capture sequence numbers remain in frame metadata.

For each active frame with valid player and hazard geometry, the parser calculates the minimum Euclidean gap between any player rectangle and any hazard rectangle. Overlapping/touching rectangles have zero clearance. The minimum pairwise gap for that frame is one clearance sample, preventing a frame with many hazards from receiving extra statistical weight. Section and level reports include arithmetic mean, nearest-rank median, minimum, and nearest-rank 10th percentile in normalized pixels. Coordinates arrive in capture pixels and all distances are scaled uniformly to a 360-pixel reference width (360 / manifest width), so equivalent captures at different widths use the same measurement scale.

Frames with valid geometry but no hazard rectangle do not produce a clearance sample. This is missing measurement, not evidence of safety. Unsupported geometry increments the section's unsupported-hitbox count and the level's unclassified-frame count.

## Explicitly not calculated yet

- 60/120/240 FPS timing-window counts: Recorder Session v1 provides a nominal capture target and timestamps, but 60 FPS images cannot prove sub-frame 120/240 FPS windows. A validated high-rate timing-event telemetry contract is required.
- Nerves: the Analyzer intentionally has no finalized Nerves formula. The parser leaves it Missing rather than inventing a score.
- Skillset tags: the current session does not include the required input/mode/event telemetry for reliable classification. No tags are guessed from decorative pixels.
- Memory classification: not implemented.
- Analyzer schema adapter: output remains a parser-owned portable report; an adapter to the Analyzer candidate's schema v3 is a later integration step.

\`parserComplete\` means all persisted frames were structurally processed and had supported geometry. It does not mean all Analyzer-required evidence (timing windows, Nerves, skillsets) is available or that a Dan can be assigned.

## Recorder dependency

The input timeline and manifest follow \`workflow-codex/level-recorder\` at \`375ddb9a35f645fb16ba055299db2a00a658e68b\`, specifically \`docs/image-parser-session-format.md\` and \`schemas/recorder-session-v1.schema.json\`. That candidate currently writes PNG frames and \`frames.csv\`, but does not yet produce \`hitboxes.csv\`. Recorder integration is required before real-session testing.
