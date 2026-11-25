# Dan Analyzer — initial architecture

## Scope

The Analyzer is the main GeoDan component. It consumes evidence produced by the image parser, level geometry inspection, and the Simulator. It does not own Geometry Dash physics or bot search. The first milestone is an evidence model and readiness gate, not a dan-scoring formula.

The priority is measurement accuracy. Do not tune a score to force a named level into a desired rank. Do not calibrate alpha–kappa in this phase.

## Current repository status (2026-11-25)

| Component | Current state |
|---|---|
| Tools | Candidate implementation mostly present; not built or runtime-tested |
| Level Recorder | Candidate implementation mostly present; not built or runtime-tested |
| Image Parser | Session contract documented; end-to-end integration not verified |
| Simulator | Hold/release trajectory candidate exists; build, side-effect isolation and gameplay behavior unverified |
| Dan Analyzer | Evidence model foundation started here; no score formula |

The Tools and Level Recorder work remains on separate isolated branches. Their status here describes candidate work, not promotion into main.

## Inputs and boundaries

1. Geometry evidence: object geometry, collision provenance, segment boundaries, hazards and clearance.
2. Simulation evidence: actual GD update/collision trial outcomes, timing windows, survival/completion, input requirements and unsupported interactions.
3. Visual evidence: obstruction/visibility and possible memory indicators. Visual evidence alone must not prove a path is impossible.
4. Gameplay telemetry: mode, position, velocity, input edges and timing only when read accurately.

Each measurement carries a source, evidence state and sample count. Unknown is distinct from zero. Unsupported measurements and interactions remain explicit; they are never silently treated as safe empty space. An unsupported metric may be preserved in the record, but it does not count as validated evidence.

## First evidence model

core/dan_analyzer_model.hpp defines:

- Game modes: classic and platformer.
- Provenance: geometry, simulation, visual, and gameplay telemetry.
- Evidence states: unknown, observed, measured, validated, unsupported.
- Segment measurements keyed by sample-index ranges (not assumed world-X spans), with minimum clearance, hazard density, timing windows, required/peak input rate, success rate and attempts to complete. Segment mode is explicit so classic and platformer evidence cannot be mixed silently.
- All 26 user-requested skillset labels.
- Initial 1st–10th Dan tier names, without mapping measurements to tiers.
- A readiness gate that refuses dan assignment unless every segment has validated geometry and simulation evidence and no unsupported interactions.

The readiness gate is deliberately conservative. It is a safety boundary, not a difficulty score. Its current requirement for validated geometry and simulation evidence in every segment is a proposed minimum-evidence rule, not a finalized scoring policy. A future calibration module may only assign a tier after its measurement features and thresholds are validated against the calibration set.

## Skillset labels

Circles, Clicksync, Fast-Paced, Timings, Chokepoints, Unbalanced, Learny, Memory, High CPS, Gimmicky, Flow, Slow-Paced, Bossfight, Mirror, Nerve Control, Cube, Ship, Ball, UFO, Wave, Robot, Spider, Old Swing, New Swing, Duals, Overall.

The model stores evidence counts/status for each label. It does not yet invent weights or infer a skillset from object counts alone. Segment sample-index ranges are independent of world X so the same contract can represent platformer movement, reversals, and non-horizontal paths. Numeric validation rejects impossible rates (for example, a success rate outside 0–1) while preserving explicitly unsupported measurements. Circles remains the user's Nine Circles / wave-focused category, not a generic circular-object detector.

## Locked decisions to preserve

- Bot search policy: single.
- Memory threshold policy: both. The exact meaning/threshold is not inferred here; visual obstruction and simulation unclear-path counts are stored separately pending the exact rule.
- Final Dan measurement policy: both. This is recorded as a locked user decision; the precise combination rule must be carried forward from the detailed design discussion rather than guessed.
- Platformer and classic must both be supported as distinct modes.
- Alpha–kappa tuning is out of scope for the first calibration pass.

## Physics-test evidence and limitations

The uploaded geodan-log(2).txt is a rotated-slope contact run. It records:
- noclip=unverified in its header, so it is not yet safe to treat every observed outcome as authoritative.
- A flat-floor grounded transition around frame 312.
- Slope/direction hook calls for object ID 1743 around frames 552–576.
- support_match=0 on the slope hook records, followed by an airborne transition around frame 577.

The uploaded geodan-log(1).txt is a separate Future Candy MEGER ship-mode run. Its header also says noclip=unverified. The player state remains stationary for the first 240 logged frames despite nonzero reported X velocity, then begins moving; around frame 246 the recorded mode changes from ship to cube, the player Y position changes sharply, and upside changes to 1. This is a useful transition/telemetry test case, but without verified noclip state and an explicit trigger/event correlation it must not be used as a validated physics measurement.

This is useful diagnostic evidence for the simulator's collision adapter and for defining what the Analyzer should record. It does not by itself establish the correct slope collision shape or whether the run is a valid no-noclip physics reference.

## Why no scoring formula yet

A score formula would currently encode unverified assumptions:
- The trajectory candidate has not been built or tested in-game.
- The rotated-slope run has unverified noclip state and ambiguous support matching.
- The recorder/parser pipeline has not been end-to-end validated.
- The calibration set is not yet a sufficiently measured, held-out dataset.

The first scoring implementation must be explainable from measured features and must expose uncertainty. It should be calibrated on known levels, then evaluated on held-out levels without changing thresholds to fit those held-out labels.

## Next implementation stages

1. Validate the evidence model with standalone tests.
2. Define a versioned parser-to-analyzer and simulator-to-analyzer evidence contract.
3. Build feature aggregation across segments, including worst-case bottlenecks rather than relying only on whole-level averages.
4. Add skillset evidence aggregation with confidence/coverage, not hard-coded object-count heuristics.
5. Build the calibration dataset and fixed dan-tier mapping for 1st–10th.
6. Only then implement dan assignment and held-out evaluation.

## Current implementation limits

- No parser, simulator, or gameplay hook is called by this core model.
- No dan rating is computed.
- No skillset is automatically classified.
- No alpha–kappa values are generated.
- No Geode integration or in-game validation is claimed.
