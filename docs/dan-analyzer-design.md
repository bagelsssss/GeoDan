# Dan Analyzer — Classic-only judge architecture

## Scope and responsibility

GeoDan analyzes Classic Geometry Dash levels only. Platformer support has been cancelled and must not be reintroduced into this pipeline, its evidence schema, or its tests.

The Dan Analyzer is the judge. It consumes:
1. Measurements from the Image Parser, which analyzes the successful recorded run using physical hitboxes.
2. Swag-route evidence from a separate route-search/simulation subsystem, which explores alternative paths not present in the recording.

The Analyzer does not replay the recorded run to measure its physics. The recording already contains the player's actual successful gameplay. Simulation is retained only where exploration is needed to discover and validate alternative routes.

The Analyzer validates evidence and eventually assigns the level's calculated Dan. This candidate implements the judge/readiness foundation, not the calibrated Dan scoring formula.

## Current implementation

| Area | State |
|---|---|
| Portable evidence model | Implemented in this candidate |
| Classic-only scope | Implemented; no game-mode field |
| Per-section clearance evidence | Modelled |
| 60/120/240 FPS frame-window evidence | Modelled and required for validated timing coverage |
| Nerves evidence | Modelled as parser output; formula not defined here |
| Parser completeness/frame accounting | Frame accounting, section ordering/ranges, and summary consistency checks implemented |
| Swag-route evidence | Modelled; confirmed easier-route bypass aggregation implemented |
| Easier-route effect | Only confirmed easier completed alternatives are counted |
| Harder-route effect | Harder alternatives do not increase difficulty |
| Final Dan formula | Not implemented |
| Geode/game integration | Not implemented or tested |

## Parser-to-Analyzer contract

The parser provides a versioned level result containing:

- Capture and parser completion flags.
- Source, processed, and unclassified frame counts. Processed plus unclassified must account for every source frame.
- Chronologically indexed sections with sample-index ranges.
- Per-section mean, median, minimum, and 10th-percentile player-to-hazard clearance in normalized pixels.
- Per-section counts of timing windows at 60, 120, and 240 FPS.
- Per-section Nerves measurement.
- Explicit unsupported-hitbox counts.
- Level-level aggregate statistics and timestamp-gap metadata.
- Skillset evidence and the two separate memory evidence channels.

Physical hitbox geometry is authoritative. Decorative artwork is not a hazard source and should not be interpreted as collision geometry. The parser must preserve unknown or unsupported hitboxes rather than silently treating them as empty space.

Clearance is measured between collision boundaries, not sprite artwork. Pixel measurements must be normalized to a stable game-space scale before comparing recordings.

Frame-window measurements must be supported by actual frame/timestamp precision. A 60 FPS image sequence alone cannot prove sub-frame 120/240 FPS windows. The parser must mark such measurements unknown or unsupported instead of interpolating fictional evidence.

The Analyzer validates the shape, completeness, provenance, and plausibility of the parser output. It does not independently recompute image recognition.

## Section and level evidence

Each active gameplay section carries:
- Mean clearance.
- Median clearance.
- Minimum clearance.
- 10th-percentile clearance.
- 60/120/240 FPS frame-window counts.
- Nerves measurement.
- Unsupported hitbox count.

Startup idle, pause, respawn, and transition sections are retained for audit but excluded from active difficulty evidence.

The Analyzer preserves section-level results. It must not allow long easy sections to erase a difficult short section through a single whole-level average.

The portable aggregation layer now produces:
- Sample-count-weighted mean of section measurements.
- Weighted median and weighted 10th percentile of the section-level values (these are distributions across sections, not exact frame-level percentiles).
- Minimum and maximum section values.
- Active section coverage and represented sample counts.
- Sums of 60/120/240 FPS frame-window counts, plus coverage counts for each rate.
- Sample-count-weighted Nerves mean and its section-level distribution.
- Count of confirmed easier alternatives and the maximum validated difficulty-bypass fraction among them.

The parser's level summary is explicitly interpreted as covering active gameplay sections only. The judge cross-checks values that can be reconstructed from section summaries: weighted clearance mean, minimum clearance, 60/120/240 window totals, Nerves weighted mean, and clearance sample count. A mismatch is rejected; missing comparison data is insufficient evidence.

The judge does not attempt to reconstruct a true whole-level frame median or frame-level 10th percentile from section medians/percentiles: those statistics cannot be exactly combined from summaries alone. The section-level median/percentile distribution is reported separately and labelled accordingly.

## Nerves

Nerves is a separate parser-provided feature, not a hidden multiplier inside clearance. It may account for the position of difficult moments in the level and preceding difficulty, but its exact formula and calibration are not invented in this model.

## Swag routes

The recorded successful route is currently a reference route, not proof of creator intent. Swag Route Detection searches for other valid routes and returns evidence with:

- Route identity and whether it is the reference or an alternative.
- Whether it completed, failed, or encountered unsupported behavior.
- Whether its measured difficulty is easier, equal, harder, or unknown relative to the reference.
- The fraction of difficulty-bearing gameplay bypassed.

Only a completed, validated easier alternative with a validated bypass fraction is counted as affecting difficulty. Harder alternatives never increase the level's difficulty. Geometric distance or elapsed time alone is not a valid bypass measure.

The aggregation reports the number of confirmed easier alternatives and the largest validated bypass fraction. It does not convert that fraction into a Dan-score adjustment yet: a calibrated rule is still needed to map bypassed difficulty-bearing gameplay to effective difficulty. The easiest confirmed route is relevant; harder routes cannot inflate the rating.

A complete route search and a completed reference route are required before the judge marks the evidence ready for calibration. Partial or unsupported searches remain insufficient evidence, not proof that no easier route exists.

## Judge behavior

The current portable judge:
- Rejects malformed schemas, impossible numeric values, fractional frame-window counts, out-of-order/overlapping sections, duplicate route IDs, inconsistent frame accounting, invalid clearance ordering, and malformed route records.
- Counts active gameplay sections while excluding idle/pause/respawn/transition sections.
- Tracks validated clearance, timing, and Nerves coverage.
- Tracks unsupported hitboxes and unsupported route outcomes.
- Counts confirmed easier alternative routes and reports their maximum validated bypass fraction.
- Aggregates section metrics and checks reconstructable level summaries against section-derived values.
- Returns Rejected, InsufficientEvidence, or ValidatedForCalibration.

ValidatedForCalibration means the evidence is structurally suitable for a later calibrated rating. It does not itself assign a Dan.

## Dan scoring and calibration

No score formula is included yet. A ranking formula must be calibrated against measured Classic levels and evaluated on a held-out set. Do not tune thresholds to force named levels into expected Dan ranks. Initial rating scope remains 1st–10th Dan; alpha–kappa tuning remains out of scope.

The eventual score should use the measured clearance distribution, timing-window burden, Nerves, and the easiest confirmed valid route. A harder swag route must never inflate the score; an easier route should reduce effective difficulty according to the difficulty-bearing gameplay it bypasses.

## Tests and limitations

The portable test covers:
- Classic-only evidence schema (there is no Platformer mode field).
- Active-section validation and idle-section exclusion.
- Weighted means, weighted section-level median/10th percentile, and minimum-clearance aggregation.
- 60/120/240 timing-window totals and Nerves aggregation.
- Parser-to-section consistency and rejection of mismatched summaries.
- Missing aggregate data, fractional frame-window counts, invalid section ranges, and invalid numeric values.
- Easier versus harder alternative routes, maximum bypass fraction, and exclusion of unvalidated bypass estimates.
- Incomplete route search, duplicate route IDs, and unsupported hitboxes.

The exact committed files are compiled by the manual GitHub Actions workflow when explicitly run. No Geode build, in-game parser integration, route-search runtime, or Dan calibration is claimed.
