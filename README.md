# GeoDan

Ranks Classic Geometry Dash levels on the osu!mania dan scale.

## Pipeline

| Part | Job | Status |
|---|---|---|
| Tools | Diagnostic overlays and analysis helpers (hitboxes, trail, trajectory) | Candidate mostly implemented; not built or runtime-tested |
| Level recorder | Records Classic gameplay on a blank background with physical hitboxes/tools available | Candidate mostly implemented; not built or runtime-tested |
| Image parser | Groups recorded images into sections and measures player-to-hazard clearance, frame windows, and Nerves | Not implemented |
| Swag route detection | Searches and validates alternative completable paths | Not implemented; simulation is reserved for route exploration |
| Dan analyzer | Judges parser and route evidence, then assigns a calibrated Dan | Portable judge foundation implemented on candidate branch; no Dan formula |

**Scope:** Classic Geometry Dash only. Platformer implementation and support are cancelled.

The Image Parser uses physical collision hitboxes as its geometry source. Decorative artwork is not treated as collision geometry. The recorded successful run is measured directly; simulation is not needed to replay it. Simulation remains relevant to discovering alternative swag routes.

## Tools (`tools/`)

Tools is being developed as a companion Geode mod. Its hitbox/trail/trajectory and recorder candidates are isolated on their respective workflow branches; they have not yet been built and validated together or promoted into the main integration workspace. Treat the pipeline table above as candidate status, not a release claim.

Build (from `tools/`): `GEODE_SDK=<path to Geode SDK> geode build --platform windows --config RelWithDebInfo --ninja`

Contract tests: `g++ -std=c++20 -Wall -Wextra -Werror -Ishared tests/test_tools_contract.cpp -o /tmp/tc && /tmp/tc`

## Status

No end-to-end in-game parser/analyzer or swag-route test has been run.

## Agent branches and implementation workflow

Agent implementation work is done on separate Git branches, not directly on `main`. The current trajectory and analyzer candidates are isolated. These remote branches are separate from local working directories; creating a remote branch does not create or modify a directory on the user's computer.

The `main` branch is the shared project baseline and documentation branch. The user-approved integration checkout is `~/main-workflow`. Agent candidates must remain isolated until the user reviews and explicitly approves promotion into `~/main-workflow`. A candidate branch is not automatically accepted or part of the actual mod.
