# Bug log

Status: `open` (no fix yet), `fixed-untested` (code change on a branch, not confirmed on the PC), `fixed` (confirmed). Branch names are the ones that hold the fix.

## Recorder (game side)

| ID | Status | Bug | Evidence | Fix plan | Branch |
|---|---|---|---|---|---|
| R1 | fixed-untested | Recording did not stop at the end of the level. There was no level-complete hook, so the recorder kept recording the end screen until the user quit. | Session `d` and the earlier `Unnamed 24` run both freeze at frame 409 and keep recording 311 and 1531 more frames | New `PlayLayer::levelComplete` hook ends the session with reason `level_complete`. A `StartGate` stops the recorder from starting a new session on the end screen until the level is restarted. The gate has a test; the hook itself needs the PC build (it is not compiled here). | `workflow-claude/bugfix-batch-2` |
| R2 | fixed | `dual_mode` in the manifest was read at session start. A level that turns on two-player later kept `false`. | Confirmed on PC: session `d` manifest says `"dual_mode": true` | Recorder remembers dual mode was seen and passes it to the worker at stop. | `workflow-claude/bugfix-batch` |
| R3 | open | Pad and portal touches always record as player 1. The `activateObject` hook doesn't say which player. | `touches.csv` `player1=1` on every pad/portal row | Find a hook that gives the player, check PC bindings. | — |
| R4 | fixed | Player 2 position and state not recorded. | Confirmed on PC: session `d` `states.csv` has world position for both players, player 2 from frame 124 | World position and player 2 columns added. | `workflow-claude/world-position` |
| R5 | open | Trigger shapes are the same green as orbs, pads and portals in the game's debug draw. | Earlier `colour_probe` run | The temporary probe (`tools/src/colour_probe.cpp`) is part of the current build, so the `colour_probe` lines should already be in your Geode log from the last recording sessions. Send that log, then a recolour rule for triggers only. Remove the probe once the rule is in. | — |
| R6 | open | 3 missed capture intervals in the two-player run, cause unknown. | `Unnamed 24` manifest | Check timing logs and frame queue. | — |
| R7 | open | Player's screen y is locked by the camera, so jumps don't show in `hitboxes.csv`. | Deadlocked: y = 540 for almost the whole run | World position logged (R4). Substage 2 depends on it. | `workflow-claude/world-position` |
| R8 | fixed | Geode code from the last changes had not been compiled. | Confirmed: PC build of `bugfix-batch` compiled and produced the new `states.csv` columns and `capture_drops.csv` | — | `workflow-claude/bugfix-batch` |
| R9 | fixed | Touch log flooding: one row per frame during contact. | 17,447 rows for about 71 contacts | `ContactFilter` (one touch per contact). | `workflow-claude/player-state` |
| R10 | fixed | `PlayerObject::touchedObject` and `playerTouchedObject` cannot be hooked on PC (inline). | PC build errors | Hooks removed. | `workflow-claude/player-state` |
| R11 | fixed | Level name written as `null`. | Earlier manifests | `level_name` written from the level. | `workflow-claude/level-name` |
| R12 | fixed | Dropped inputs, touches and no-buffer frames were only written to the game log. | Confirmed on PC: session `d` has `capture_drops.csv` (all zero) and the analyzer reports `lossy 0` | Recorder writes `capture_drops.csv`; analyzer reads it. | `workflow-claude/bugfix-batch` |
| R14 | fixed | Every frame was unsupported because hitbox types 22 and 30 were unclassified. | Early Deadlocked sessions had no usable frames | Recorder ignore list for types 22 and 30 (noted per frame as `ignored_type`). | `workflow-claude/ignore-list` |
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
| A7 | open | Substage 2 (video check) is not implemented. It is now unblocked: world position is logged (R4), presses map to the right frame (A9) and the end screen is trimmed (A10). | Session `d` has `world_y` for both players | Detect takeoffs from world y and compare with presses; flag presses with no response and takeoffs with no press. Player 2 is noisy in session `d` (orbs and pads cause rises), so use orb and pad touches to explain takeoffs. | — |
| A8 | open | The analyzer doesn't use `states.csv` for anything except dual detection. Mode and gravity are unused. | Code | Add to the verification step. | — |
| A9 | fixed | A press was assigned to the frame before the one that shows it. | Session `d`, player 1: with the old rule 6 of 6 takeoffs were 1 frame late; with the new rule 6 of 6 land on the assigned frame. Player 2: 2 at 0, 1 at +1, 1 at +3 (never earlier; the later ones are probably orbs or pads). | An input is assigned to the first frame stamped at or after it (a frame is stamped at capture, so a later input first shows in the next frame). An input after the last stamp belongs to the last frame. `unmatchedInputs` is now always 0. | `workflow-claude/bugfix-batch-2` |
| A10 | fixed | The analyzer counted the frozen end screen as gameplay, and counted clicks made on it. | Session `d`: 311 of 721 frames frozen after frame 409; 4 end-screen inputs. The older `Unnamed 24` run freezes at the same frame 409 (1531 frozen frames). | `activeFrameCount` finds the last frame where any player moved (from `states.csv`). A tail of at least half a second is the end screen: it is left out of blocks, sections, clearance and press counts, and reported as `endScreenFrames` and `endScreenInputs`. Not trimmed when `states.csv` is missing, has no position columns, nothing ever moved, or the tail is shorter than half a second. | `workflow-claude/bugfix-batch-2` |

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

## Merging

- The merge target is `workflow-claude/master`. The stack is a fast-forward of it (25 commits ahead, 0 behind at the time of writing). `main` is a different branch and is not touched.
- The stack includes the temporary colour probe (`tools/src/colour_probe.cpp`, commit `dc8f2b6`, see R5). It hooks `CCDrawNode::drawPolygon` and logs up to 500 lines. Decide whether to merge it now or remove it first.

## Change log

Every commit on the stack since `workflow-claude/master`, with the bug IDs it addresses and the files it changed. `—` means a feature or groundwork commit, not a bug fix.

| Commit | Bug IDs | What changed | Files |
|---|---|---|---|
| `95af657` | — | Record button presses to inputs.csv for the I/I Analyzer | `core/recorder_input_log.hpp`, `tests/test_recorder_input_log.cpp`, `tools/src/overlay.cpp`, `tools/src/recorder_live.cpp`, `tools/src/recorder_live.hpp` |
| `39c0a5d` | — | Hook button presses on GJBaseGameLayer; show input count in the recorder label | `tools/src/overlay.cpp`, `tools/src/recorder_live.cpp`, `tools/src/recorder_live.hpp` |
| `5e56be0` | — | Add input_file to the v2 manifest; add the input log reader and checks | `core/input_session.hpp`, `core/recorder_session.hpp`, `docs/image-parser-session-format.md`, `schemas/recorder-session-v2.schema.json`, `tests/test_input_session.cpp`, `tests/test_recorder_video_worker.cpp` |
| `0578815` | — | Add the v2 I/I Analyzer core: 100 blocks of clearance and presses | `core/input_image_analyzer.hpp`, `tests/test_input_image_analyzer.cpp` |
| `879c229` | — | Record why frames are unsupported (unsupported_reasons.csv) | `core/recorder_geometry.hpp`, `core/recorder_unsupported_log.hpp`, `tests/test_recorder_unsupported_log.cpp`, `tools/src/recorder_geometry.hpp`, `tools/src/recorder_live.cpp` |
| `2e8606c` | R11 | Log the level name in the v2 manifest | `core/input_image_analyzer.hpp`, `core/recorder_session.hpp`, `docs/image-parser-session-format.md`, `schemas/recorder-session-v2.schema.json`, `tests/test_input_image_analyzer.cpp`, `tests/test_recorder_video_worker.cpp`, `tools/src/recorder_live.cpp` |
| `e57e55e` | R14 | Ignore SecretCoin and Collectible in the recorder's unsupported rule | `core/hitbox_model.hpp`, `tests/test_hitbox_model.cpp`, `tests/test_recorder_unsupported_log.cpp`, `tools/src/recorder_geometry.hpp` |
| `dc8f2b6` | R5 (diagnostic) | Add a temporary colour probe for game debug hitboxes | `tools/src/colour_probe.cpp` |
| `5ab1396` | — | Add 5-second sections to the I/I Analyzer | `core/input_image_analyzer.hpp`, `tests/test_input_image_analyzer.cpp` |
| `720bb6a` | A6 (provisional) | Add a provisional difficulty model and dan labels | `core/difficulty_model.hpp`, `tests/test_difficulty_model.cpp` |
| `c9e6b86` | A6 (provisional) | Add a per-press term to the difficulty model | `core/difficulty_model.hpp`, `tests/test_difficulty_model.cpp` |
| `c73f750` | — | Record player state and orb, pad and portal touches | `core/recorder_state_log.hpp`, `tests/test_recorder_state_log.cpp`, `tools/src/overlay.cpp`, `tools/src/recorder_live.cpp`, `tools/src/recorder_live.hpp` |
| `b29fcc8` | R10 | Hook pad and portal touches on PlayerObject::touchedObject | `tools/src/overlay.cpp` |
| `cf1292d` | R10 | Record orb touches only for now | `tools/src/overlay.cpp` |
| `b8b91ee` | R10 | Record pad and portal touches from GameObject::activateObject | `tools/src/overlay.cpp` |
| `4374d11` | R9 | Record one touch per contact | `core/recorder_state_log.hpp`, `tests/test_recorder_state_log.cpp`, `tools/src/overlay.cpp`, `tools/src/recorder_live.cpp`, `tools/src/recorder_live.hpp` |
| `4296c1e` | T1 | Fix the contact-filter test variable names | `tests/test_recorder_state_log.cpp` |
| `e4f89e4` | A1 | Single-player runs pair by button alone; recorder writes `dual_mode`; schema gains `dual_mode`. | `core/input_image_analyzer.hpp`, `core/input_session.hpp`, `core/recorder_session.hpp`, `schemas/recorder-session-v2.schema.json`, `tests/test_input_session.cpp`, `tools/src/recorder_live.cpp` |
| `a4797b0` | A3 | Press and release counts per player, per block, per section and for the run. | `core/input_image_analyzer.hpp`, `tests/test_input_image_analyzer.cpp` |
| `1f3b3bb` | R4, R7 | `states.csv` gains `world_x`, `world_y`, `mode_p2`, `world_x_p2`, `world_y_p2`. | `core/recorder_state_log.hpp`, `tests/test_recorder_state_log.cpp`, `tools/src/recorder_live.cpp` |
| `6182eb2` | A2 | Analyzer treats a run as dual when the manifest or `states.csv` says so. | `core/input_image_analyzer.hpp`, `tests/test_input_image_analyzer.cpp` |
| `fad5309` | docs | Add bug log covering recorder, analyzer and tooling | `docs/BUGLOG.md` |
| `b250b79` | docs | Add two more bugs to the bug log | `docs/BUGLOG.md` |
| `eaa7062` | R2, R12, A4, A5 | Dual flag passed to the worker at stop and written to the manifest; `capture_drops.csv` written by the recorder and read by the analyzer (`lossy`, drop counts); geometry looked up by sequence; player 2 column found by header name. New: `core/recorder_drops.hpp`, `tests/test_recorder_drops.cpp`. | `core/input_image_analyzer.hpp`, `core/recorder_drops.hpp`, `core/recorder_session.hpp`, `core/recorder_worker.hpp`, `docs/BUGLOG.md`, `tests/test_input_image_analyzer.cpp`, `tests/test_recorder_drops.cpp`, `tests/test_recorder_video_worker.cpp`, `tools/src/recorder_live.cpp` |
| `4f71cb2` | docs | Update bug log from the first bugfix-batch PC session | `docs/BUGLOG.md` |
| `68e5242` | R1, A9, A10 | `PlayLayer::levelComplete` hook and `StartGate` (new `core/recorder_start_gate.hpp`, test `tests/test_recorder_start_gate.cpp`); analyzer `frameShowingEvent` (A9) and `activeFrameCount` with new result fields `analyzedFrames`, `endScreenFrames`, `endScreenInputs` (A10); analyzer tests updated and extended. | `core/input_image_analyzer.hpp`, `core/recorder_start_gate.hpp`, `tests/test_input_image_analyzer.cpp`, `tests/test_recorder_start_gate.cpp`, `tools/src/overlay.cpp`, `tools/src/recorder_live.cpp`, `tools/src/recorder_live.hpp` |
| (this commit) | docs | Bug log: statuses for R1, A9, A10; new R14; R5 and A7 notes; this change log and the merge notes. | `docs/BUGLOG.md` |
