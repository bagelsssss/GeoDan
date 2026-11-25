# GeoDan

Ranks Geometry Dash levels on the osu!mania dan scale.

## Pipeline

| Part | Job | Status |
|---|---|---|
| Tools | Diagnostic overlays and analysis helpers (hitboxes, trail, trajectory) | Mostly implemented candidate; not built or in-game tested (isolated branch) |
| Level recorder | Records the level on a blank background with the tools on | Mostly implemented candidate; not built or in-game tested (isolated branch) |
| Image parser | Turns the recorder's video into per-frame images the analyzer can read | planned |
| Simulator | Plays the level by trial and error with no user input | planned |
| Analyzer | Calculates the dan from a fixed dan list. The main mod | planned |

The old repo, `GeoDan-deprecated`, is archived. Some of its code is being ported as each part is started.

## Tools (`tools/`)

Tools is being developed as a companion Geode mod. Its current implementation is a candidate on workflow-codex/trajectory-rebuild; the separate Level Recorder candidate is on workflow-codex/level-recorder. Neither candidate has been built and runtime-validated together or promoted into the user's main integration workspace.

Build (from `tools/`): `GEODE_SDK=<path to Geode SDK> geode build --platform windows --config RelWithDebInfo --ninja`

Contract tests: `g++ -std=c++20 -Wall -Wextra -Werror -Ishared tests/test_tools_contract.cpp -o /tmp/tc && /tmp/tc`

## Status

Nothing here has been run in game yet.


## Agent branches and implementation workflow

Agent implementation work is done on separate Git branches, not directly on `main`. For the current trajectory rebuild, Codex has a candidate branch named `workflow-codex/trajectory-rebuild`, created from the current `main` commit. Claude should use a separate candidate branch for its own trajectory work if/when that work begins.

These remote branches are separate from the local working directories described below. Creating a remote branch does not create or modify a directory on the user's computer.

The `main` branch is the shared project baseline and documentation record; it is not the place for experimental agent implementation. The user-approved integration checkout is `~/main-workflow`. Agent candidates must remain isolated until the user reviews and explicitly approves promotion into `~/main-workflow`. A candidate branch is not automatically accepted or part of the actual mod.
