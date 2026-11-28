# Bug log

Status: `open` (no fix yet), `fixed-untested` (code change on a branch, not confirmed on the PC), `fixed` (confirmed). Branch names are the ones that hold the fix.

## Recorder (game side)

| ID | Status | Bug | Evidence | Fix plan | Branch |
|---|---|---|---|---|---|
| R1 | open | Recording does not stop at the end of the level. The recorder has no level-complete hook, so it keeps recording the end screen until the user quits. Cause confirmed by the `d` session (below). | Session `d` (`Unnamed 24`, dual): both players' world x freeze at frame 409 (6.82 s); 311 of 721 frames follow, stop reason `user_quit` | Hook `PlayLayer::levelComplete`, stop with `level_complete`, and keep the recorder from starting a new session on the end screen until the level is restarted. Needs the PC bindings check (hookable on PC?). | — |
| R2 | fixed | `dual_mode` in the manifest was read at session start. A level that turns on two-player later kept `false`. | Confirmed on PC: session `d` manifest says `"dual_mode": true` | Recorder remembers dual mode was seen and passes it to the worker at stop. | `workflow-claude/bugfix-batch` |
| R3 | open | Pad and portal touches always record as player 1. The `activateObject` hook doesn't say which player. | `touches.csv` `player1=1` on every pad/portal row | Find a hook that gives the player, check PC bindings. | — |
| R4 | fixed | Player 2 position and state not recorded. | Confirmed on PC: session `d` `states.csv` has world position for both players, player 2 from frame 124 | World position and player 2 columns added. | `workflow-claude/world-position` |
| R5 | open | Trigger shapes are the same green as orbs, pads and portals in the game's debug draw. | Earlier `colour_probe` run | Need probe output from PC, then a recolour rule for triggers only. | — |
| R6 | open | 3 missed capture intervals in the two-player run, cause unknown. | `Unnamed 24` manifest | Check timing logs and frame queue. | — |
| R7 | open | Player's screen y is locked by the camera, so jumps don't show in `hitboxes.csv`. | Deadlocked: y = 540 for almost the whole run | World position logged (R4). Substage 2 depends on it. | `workflow-claude/world-position` |
| R8 | fixed | Geode code from the last changes had not been compiled. | Confirmed: PC build of `bugfix-batch` compiled and produced the new `states.csv` columns and `capture_drops.csv` | — | `workflow-claude/bugfix-batch` |
| R9 | fixed | Touch log flooding: one row per frame during contact. | 17,447 rows for about 71 contacts | `ContactFilter` (one touch per contact). | `workflow-claude/player-state` |
| R10 | fixed | `PlayerObject::touchedObject` and `playerTouchedObject` cannot be hooked on PC (inline). | PC build errors | Hooks removed. | `workflow-claude/player-state` |
| R11 | fixed | Level name written as `null`. | Earlier manifests | `level_name` written from the level. | `workflow-claude/level-name` |
| R12 | fixed | Dropped inputs, touches and no-buffer frames were only written to the game log. | Confirmed on PC: session `d` has `capture_drops.csv` (all zero) and the analyzer reports `lossy 0` | Recorder writes `capture_drops.csv`; analyzer reads it. | `workflow-claude/bugfix-batch` |
| R13 | open | Player hidden on a repeating pattern at spawn: frames 0-2, 6-8 and 12-14 are `player_hidden` in session `d`. Cause unknown (spawn blink?). | `unsupported_reasons.csv`: `player_hidden` 9 frames | Check the first frames of the video, and whether the hidden frames are a spawn animation. Low priority. | — |

## Analyzer (I/I)

| ID | Status | Bug | Evidence | Fix plan | Branch |
|---|---|---|---|---|---|
| A1 | fixed | Single-player keyboard logs have changing player flags, so pairing by (button, player) rejected valid runs. | Analyzer accepts the keyboard and mouse sessions | Pair by button alone in single-player runs. | `workflow-claude/input-pairing` |
| A2 | fixed | Two-player logs rejected when the manifest flag was stale (see R2). | Analyzer reads the old `Unnamed 24` session as dual from `states.csv` | Dual detection from `states.csv`. | `workflow-claude/dual-detect` |
| A3 | fixed | No per-player clicks. | Session `d`: 12 presses each for player 1 and player 2 | Per-player press and release counts. | `workflow-claude/dual-analyzer` |
| A4 | fixed | `analyzeSession` found each frame's geometry by linear search (O(n²)). | Output identical on the keyboard, mouse, two-player and Deadlocked sessions | Lookup by sequence in a hash map. Duplicate sequences are now an error. | `workflow-claude/bugfix-batch` |
| A5 | fixed | `statesHavePlayer2` read the player 2 column by position. | Old `Unnamed 24` session still detected as dual after the change | Column found by header name. | `workflow-claude/bugfix-batch` |
| A6 | open | Difficulty model fitted to one anchor. Press weight 0.25 is a guess. Scores are provisional. | Deadlocked = 6.50 only | Revised anchor list from user, then refit. | — |
| A7 | open | Substage 2 (video check) is not implemented. World position is now logged (R4), so the check can be built. | Session `d` has `world_y` for both players | Detect takeoffs from world y and compare with presses; flag presses with no response and takeoffs with no press. Depends on A9 and A10. | — |
| A8 | open | The analyzer doesn't use `states.csv` for anything except dual detection. Mode and gravity are unused. | Code | Add to the verification step. | — |
| A9 | open | A press is assigned to the frame before the one that shows it. Confirmed for player 1; consistent for player 2; small sample. | Session `d`: player 1, 6 of 6 takeoffs appear exactly 1 frame after the matched frame (none at 0). Player 2: 2 at +1, 1 at +2, none at 0. | Match a press to the first frame at or after its time, instead of the last frame at or before it. Confirm on a longer run first. | — |
| A10 | open | The analyzer counts the frozen end screen as gameplay, and counts clicks made on it. | Session `d`: 311 of 721 frames (43%) are frozen after frame 409 yet marked `active_gameplay` and `valid`; 4 inputs (frames 414, 421, 423, 430) fall after the freeze | Trim trailing frames where no player's world position changes (needs `states.csv`), and report end-screen inputs separately. Also fixed at the source by R1. | — |

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
| T8 | open | 10 of 19 portable tests are not run by any CI workflow: `test_difficulty_model`, `test_input_image_analyzer`, `test_input_session`, `test_recorder_drops`, `test_recorder_input_log`, `test_recorder_state_log`, `test_recorder_unsupported_log`, `test_recorder_video_sink`, `test_recorder_video_worker`, `test_recorder_worker_sink`. | grep of `.github/workflows` | Add them to a workflow (needs your OK before CI runs are triggered). | — |

## Not bugs (for the record)

- The Dan icon "κ" was present. Flag dropped.
- Zero-sample blocks in Deadlocked are real: no hazards on screen in those frames.
