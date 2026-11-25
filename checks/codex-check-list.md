# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Read `checks/README.md` and `checks/claude-check-list.md` before project work.

## Current status

- **Status:** Shared development candidate assembled on `alpha-branch`; unfinished components are intentionally retained.
- **Current branch:** `alpha-branch`.
- **Baseline:** `main` at `0e7a3a7fb6ac0995ad9d176d0c1ccd63b1b585bf`.
- **User decision:** Codex and Claude may contribute to `alpha-branch` as the shared candidate. Coordinate before overlapping edits and preserve each contributor's provenance.
- **Promotion:** Not approved for `main` or `~/main-workflow`.
- **Local workspace:** User's local repositories were not accessed.

## Included Codex workstreams

1. **Trajectory** — `workflow-codex/trajectory-rebuild` at `bd4498f51d43ff24f5c0bebb9cbbb83f2801b5d1`.
   - Includes the model, Geode-facing hidden-player simulator, collision filters, hooks, overlay renderer integration, tests, and docs.
   - Known issue: candidate's trajectory source has a compile error at the end of `tools/src/trajectory.cpp`; the timestep is frame-time based. Neither is silently fixed here.
2. **Level Recorder** — `workflow-codex/level-recorder` at `a1563a3d719d196f5fac1c545f6a1f735de2546e`.
   - Includes capture timing/backpressure model, pixel buffer pool, PNG encoder, worker/session writer, hitbox sidecar writer, geometry adapter, portrait target/preview, tests, and docs.
   - Known issue: Codex candidate still has no live PlayLayer producer or GPU pixel readback; tool remains planned. Geometry uses provisional AABBs and marks unsupported cases.
3. **Image Parser** — `workflow-codex/image-parser` at `44b8fdb0eed5a740ee63776b94d9ab44bbf22c35`.
   - Includes session validation, hitbox sidecar parsing, frame sections, clearance metrics, tests, and docs.
   - Known issue: no actual live capture pipeline or Analyzer adapter; some telemetry and classification are intentionally missing.
4. **Dan Analyzer** — `workflow-codex/dan-analyzer` at `2085263aa9e6bbd6d45045cf8a039e9fb8e3e4cd`.
   - Includes Classic-only evidence aggregation, validation, route-comparison primitives, score-scale/calibration scaffolding, tests, and docs.
   - Known issue: the latest test suite was reported failing by Claude at `tests/test_dan_analyzer_model.cpp:229`; scoring weights and calibrated curve remain undefined.

## Shared Tools integration

- Latest `main` remains the baseline for the existing Tools button, popup, tool registry, hitbox model/trail and other shared files.
- Recorder's shared overlay renderer and portrait-preview path are included.
- Trajectory hooks have been integrated into that Recorder overlay: per-frame update, reset, and quit cleanup.
- `tools/mod.json` retains both the Recorder portrait-preview diagnostic setting and the Trajectory setting.
- The shared overlay/mod integration is source-composed, not Geode-compiled or in-game verified.

## Validation status

- Portable test evidence exists on source candidates, but not all exact latest candidate suites have been rerun after assembly.
- Claude's latest reported Dan Analyzer test failure is retained and must be investigated.
- No combined Geode build, GitHub Actions run, or in-game validation has been performed on `alpha-branch`.
- Do not treat this branch as buildable or the analyzer pipeline as end-to-end until validated.

## Handoff

- Review the composed `tools/src/overlay.cpp` and `tools/mod.json` before further edits.
- Resolve the Trajectory compile/timestep issues and Dan Analyzer failing test.
- Wire and validate Recorder live capture/readback; then run Parser on an actual captured session and connect Parser output to Analyzer.
- Keep `main` and `~/main-workflow` untouched unless the user separately approves promotion.
