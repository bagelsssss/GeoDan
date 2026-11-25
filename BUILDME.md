# BUILDME — build and test GeoDan candidates

Status: draft from Claude's findings. Codex's independent findings are **not yet combined** into this file. Replace this line when they are.

Nothing in this file has been run in the game. Portable tests were run on Ubuntu 24.04 (g++ 13.3). Your machine uses g++ 16.2.1, so re-run the tests there first.

## 1. Verified project state

Heads checked on this date with `git ls-remote --heads origin`:

| Branch | Head | What it contains | Status |
|---|---|---|---|
| `main` | `96bdf6a` | Baseline: hitbox model, tool registry, Tools popup | Reference |
| `workflow-claude/recorder-live-from-codex` | `860be16` | Recorder library + live capture wired into the overlay | Build candidate: recorder |
| `workflow-claude/trajectory-from-codex` | `6a8741b` | Trajectory model + Geode trajectory, `;` fix, fixed 1/240 step | Build candidate: trajectory |
| `workflow-codex/trajectory-rebuild` | `bd4498f` | Codex's trajectory | **Does not compile** (`trajectory.cpp:435` missing `;`); frame-time step. Do not build. |
| `workflow-codex/level-recorder` | `a1563a3` | Recorder library, no live capture | Portable tests only |
| `workflow-claude/level-recorder-from-codex` | `47f3cb5` | Recorder library | Portable tests only; superseded by the recorder-live branch |
| `workflow-codex/image-parser` | `44b8fdb` | Image parser | Portable tests only |
| `workflow-claude/image-parser-from-codex` | `04bf467` | Image parser, test fix | Portable tests only |
| `workflow-codex/dan-analyzer` | `2085263` | Analyzer model | **Test fails** at `tests/test_dan_analyzer_model.cpp:229`. Not wired into the mod. |

Build separately: the recorder branch and the trajectory branch both change `tools/src/overlay.cpp`. They are not compatible in one build.

## 2. Prerequisites (verified on Arch)

| Tool | Verified version |
|---|---|
| git | 2.56.1 |
| g++ | 16.2.1 |
| cmake | 4.5.0 |
| ninja | 1.13.2 |
| python3 | 3.14.10 |
| geode CLI | 4.0.1 (reports SDK 5.11.2 via `geode sdk version`) |
| Geode SDK | 5.11.2 at `/home/bagel/.local/share/Geode/sdk` |

The mod targets Geode 5.11.2 (`tools/mod.json`), which matches the SDK. The CLI is older, so check the first build's messages for version complaints.

The Windows build from Linux needs cross-compilation tools (`geode sdk install-linux`). Not yet installed. It downloads a Windows SDK, so run it only after you approve it.

## 3. Command blocks

Replace `~/GeoDan` with your clone's path.

### A. Preflight — verified on Arch

```bash
set -euo pipefail
for t in git g++ cmake ninja python3 geode; do
  printf '%-8s ' "$t"
  if command -v "$t" >/dev/null; then "$t" --version 2>/dev/null | head -1 || true; else echo MISSING; fi
done
echo "GEODE_SDK=${GEODE_SDK:-<not set>}"
geode sdk version
```

### B. Candidate worktrees — ready, not yet run

```bash
set -euo pipefail
cd ~/GeoDan
git fetch origin
BASE="$HOME/geodan-candidates"
mkdir -p "$BASE"
for b in main workflow-codex/level-recorder workflow-claude/recorder-live-from-codex \
         workflow-codex/image-parser workflow-claude/trajectory-from-codex workflow-codex/dan-analyzer; do
  n=$(echo "$b" | tr / _)
  [ -d "$BASE/$n" ] || git worktree add --detach "$BASE/$n" "origin/$b"
done
ls "$BASE"
```

### C. Portable tests — verified on Ubuntu, not yet on Arch

```bash
set -euo pipefail
BASE="$HOME/geodan-candidates"
run_suite() {
  local d="$BASE/$1" rc=0
  for t in "$d"/tests/test_*.cpp; do
    local name; name=$(basename "$t" .cpp)
    echo "== [$1] $name"
    if g++ -std=c++20 -Wall -Wextra -Werror -I"$d/core" -I"$d" "$t" -o "$d/bin_$name"; then
      "$d/bin_$name" && echo "   PASS" || { echo "   RUN FAIL"; rc=1; }
    else echo "   COMPILE FAIL"; rc=1; fi
  done
  return $rc
}
for s in main workflow-codex_level-recorder workflow-claude_recorder-live-from-codex \
         workflow-codex_image-parser workflow-claude_trajectory-from-codex workflow-codex_dan-analyzer; do
  run_suite "$s" || echo "SUITE $s HAS FAILURES"
done
```

Expected: `PASS` for every test, except `workflow-codex_dan-analyzer`, which fails at `test_dan_analyzer_model.cpp:229`.

### D. Geode build — not yet run

```bash
set -euo pipefail
cd "$HOME/geodan-candidates/workflow-claude_recorder-live-from-codex/tools"
export GEODE_SDK="/home/bagel/.local/share/Geode/sdk"
test -f "$GEODE_SDK/CMakeLists.txt" || { echo "GEODE_SDK is wrong"; exit 1; }
geode build --platform windows --config RelWithDebInfo --ninja
```

The `--platform windows` flag, `--ninja`, and `--config` are confirmed in `geode build --help`. Whether the Linux-to-Windows build succeeds needs the cross-compilation tools from `geode sdk install-linux` first. Build the trajectory candidate the same way, from its own worktree.

Do not copy the built mod into the game's Mods folder until the build succeeds.

### E. In-game test — not yet run

- Settings: `hitboxes`, `hitbox-trail`, `unclassified`, `trajectory` (trajectory build only), `level-recorder`, and `portrait-preview-test` (diagnostic).
- Level: any level. The project does not name one.
- Hitboxes and trail: enable them; hitboxes should outline objects and the trail should show behind the player.
- Recorder: enable Level recorder, play a few seconds, then restart. Each restart starts a new session.
- Output: `recordings/session_<unix ns>/` containing `manifest.json`, `frames.csv`, `hitboxes.csv`, and `frames/frame_000000.png`. Find it with:
  `find ~/.local/share/Steam/steamapps/compatdata/322170 -type d -name recordings 2>/dev/null`
- Pass: the folder exists, the PNGs open at 360×1080, and the game doesn't stutter badly.
- Fail: send the Geode log (search for `GeoDan`), `manifest.json`, and a screenshot of the portrait preview.

## 4. Known blockers

- No Geode build has run yet on this project. Every in-game result is untested.
- `workflow-codex/trajectory-rebuild` does not compile and still uses frame time for its step.
- `workflow-codex/dan-analyzer` fails its own test suite.
- `portrait-preview-test` fails the `check_commit.py` settings-match check. It is a temporary diagnostic setting, not a registry tool. Decision pending.
- The `README.md` contract-test command points at a `shared/` folder and `tests/test_tools_contract.cpp`. Neither exists on `main`.
- Codex has committed newer work since Claude's last review. That work is not yet reviewed.

## 5. Structure of this file

1. Verified project state
2. Prerequisites
3. Command blocks A–E
4. Known blockers
5. Codex's combined findings (pending)
