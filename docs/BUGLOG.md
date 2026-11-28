# Bug log

Status: `open` (no fix yet), `fixed-untested` (code change on a branch, not confirmed on the PC), `fixed` (confirmed). Branch names are the ones that hold the fix.

## Recorder (game side)

| ID | Status | Bug | Evidence | Fix plan | Branch |
|---|---|---|---|---|---|
| R1 | open | Deadlocked session stopped with `user_quit` while the player was alive. Unknown whether the level finished or the user quit. | `4c3134bb` manifest, last frame active gameplay at world x 12083 | Ask user. If the level finished, hook `PlayLayer::levelComplete` and stop with `level_complete`. | — |
| R2 | fixed-untested | `dual_mode` in the manifest is read at session start. A level that turns on two-player later keeps `false`. | `Unnamed 24` run: dual from frame 124, manifest `false` | Analyzer reads `states.csv` too (done). Recorder should set the flag when dual mode turns on, thread-safe. | `workflow-claude/dual-detect` (analyzer side) |
| R3 | open | Pad and portal touches always record as player 1. The `activateObject` hook doesn't say which player. | `touches.csv` `player1=1` on every pad/portal row | Find a hook that gives the player, check PC bindings. | — |
| R4 | fixed-untested | Player 2 position and state not recorded. | `states.csv` had player 1 only | Added world position and player 2 columns. | `workflow-claude/world-position` |
| R5 | open | Trigger shapes are the same green as orbs, pads and portals in the game's debug draw. | Earlier `colour_probe` run | Need probe output from PC, then a recolour rule for triggers only. | — |
| R6 | open | 3 missed capture intervals in the two-player run, cause unknown. | `Unnamed 24` manifest | Check timing logs and frame queue. | — |
| R7 | open | Player's screen y is locked by the camera, so jumps don't show in `hitboxes.csv`. | Deadlocked: y = 540 for almost the whole run | World position logged (R4). Substage 2 depends on it. | `workflow-claude/world-position` |
| R8 | open | Geode code from the last changes has not been compiled. | Sandbox has no GD headers | PC build of each branch. | `world-position`, `dual-analyzer` |
| R9 | fixed | Touch log flooding: one row per frame during contact. | 17,447 rows for about 71 contacts | `ContactFilter` (one touch per contact). | `workflow-claude/player-state` |
| R10 | fixed | `PlayerObject::touchedObject` and `playerTouchedObject` cannot be hooked on PC (inline). | PC build errors | Hooks removed. | `workflow-claude/player-state` |
| R11 | fixed | Level name written as `null`. | Earlier manifests | `level_name` written from the level. | `workflow-claude/level-name` |

## Analyzer (I/I)

| ID | Status | Bug | Evidence | Fix plan | Branch |
|---|---|---|---|---|---|
| A1 | fixed-untested | Single-player keyboard logs have changing player flags, so pairing by (button, player) rejects valid runs. | Keyboard run: `player1=0` on 154 of 155 rows | Pair by button alone in single-player runs. | `workflow-claude/input-pairing` |
| A2 | fixed-untested | Two-player logs rejected when the manifest flag is stale (see R2). | `Unnamed 24` gave 6 alternation errors in single mode | Dual detection from `states.csv`. | `workflow-claude/dual-detect` |
| A3 | fixed-untested | No per-player clicks. | Only one count per block/section | Per-player press and release counts. | `workflow-claude/dual-analyzer` |
| A4 | open | `analyzeSession` finds each frame's geometry with a linear search, which is O(n²). Slow on long runs. | `core/input_image_analyzer.hpp`, geometry matching loop | Index geometry by sequence number. | — |
| A5 | open | `statesHavePlayer2` reads column 8 by position. Breaks if the column order changes. | `core/input_image_analyzer.hpp` helper | Look columns up by header name. | — |
| A6 | open | Difficulty model fitted to one anchor. Press weight 0.25 is a guess. Scores are provisional. | Deadlocked = 6.50 only | Revised anchor list from user, then refit. | — |
| A7 | open | Substage 2 (video check) is not implemented. The screen-y method cannot work. | See R7 | Use world y once R4 is confirmed on PC. | — |
| A8 | open | The analyzer doesn't use `states.csv` for anything except dual detection. Mode and gravity are unused. | Code | Add to the verification step. | — |

## Tooling and docs

| ID | Status | Bug | Evidence | Fix plan | Branch |
|---|---|---|---|---|---|
| T1 | fixed | A pipe hid a compile failure, and a broken test was committed. | Commit `4296c1e` | Check exit codes, not pipe output. | `workflow-claude/player-state` |
| T2 | fixed | My own test runs printed "passed" from an old binary after a failed compile. | Analyzer test, this session | Chain compile and run with `&&`. | — |
| T3 | fixed | PC checkout pulled the wrong branch. | Earlier PC session | Name the branch in every command. | — |
| T4 | open | `BUILD.md` points at commit `4d12f81`, an old commit. | `BUILD.md` | Update to current branch and steps. | — |
| T5 | open | `BUILDME.md` lists old branch names and says nothing has been run in the game. | `BUILDME.md` | Rewrite the branch table. | — |
| T6 | open | `states.csv` columns are not in the schema or any doc. | `schemas/` has no states entry | Document the columns. | — |
| T7 | open | Sandbox Geode bindings are older than the PC's, so hook checks there can be wrong. | Earlier build failures | Check hooks against PC headers. | — |

## Not bugs (for the record)

- The Dan icon "κ" was present. Flag dropped.
- Zero-sample blocks in Deadlocked are real: no hazards on screen in those frames.
