# GeoDan

Ranks Geometry Dash levels on the osu!mania dan scale.

## Pipeline

| Stage | Job | Status |
|---|---|---|
| Tools | Diagnostic overlays (hitboxes, trail) and the level recorder's tool set | Done |
| Level recorder | Records the level to `video.mp4` with the tools on, matching the on-screen preview | Done |
| Input/Image Analyzer (I/I Analyzer) | Compares the player's input with the preview video, block by block (100 blocks), and produces the evidence the dan analyzer reads | Planned |
| Dan analyzer | Takes the I/I Analyzer results into the dan system and returns a dan rank | Planned |
| Simulator (submod) | A robot that plays the level in place of human input. Trajectory prediction lives here, not in the tools mod | Planned |

Order: I/I Analyzer → test → dan analyzer → test → simulator. Once the simulator reproduces the manual results, its output replaces user input.

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
