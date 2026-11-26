# Build and test: MP4 recorder with FFmpeg API (PC, Proton)

## The commit to test

**`4d12f81`** on `workflow-claude/recorder-video`.

The recorder encodes the level in-process through the **FFmpeg API** mod, so it no longer needs an `ffmpeg.exe` on the PATH. This is the change that fixes the missing `video.mp4` under Proton.

Not yet on master. Master gets it only after you test it.

## 1. Requirements (Steam, Proton)

| Item | Notes |
|---|---|
| Geometry Dash with Geode | the Windows build under Proton |
| FFmpeg API (`eclipse.ffmpeg-api`) | install from the Geode mod index; GeoDan Tools needs it (required, version 1.2.2 or newer) |
| Build machine | Geode SDK 5.11.2 and a Windows build of the mod |

Status: none of these steps have been run on your PC yet.

## 2. Check out the commit

```bash
git clone https://github.com/bagelsssss/GeoDan.git
cd GeoDan
git fetch origin
git checkout 4d12f81
git log --oneline -1
```

The last line must start with `4d12f81`.

## 3. Build the mod

```bash
cd tools
geode build --config RelWithDebInfo
```

The `.geode` file is written under `tools/build`. Copy it into the game's Mods folder only after a successful build, and install FFmpeg API before starting the game.

If the build cannot find the FFmpeg API headers, send the full error. The header include and dependency wiring are not confirmed by a build yet.

## 4. In-game test

1. Start the game and check that both GeoDan Tools and FFmpeg API are loaded (Geode menu).
2. In the Tools popup, turn on **Level recorder**.
3. Open a level, play for about 5 seconds, then restart. Each restart starts a new session.
4. Find the session folder `local.geodan-tools/recordings/session_<unix ns>`.
5. Check the folder:
   - `video.mp4` exists and plays in a video player.
   - `manifest.json` has `"schema_version": 2` and `"status": "complete"`.
   - `frames.csv` has one row per frame.

If `video.mp4` is missing, send the `manifest.json` (its `stop_reason` names the failing step) and the Geode log lines that mention `GeoDan` or `FFmpeg`.

## 5. Portable tests (optional, no game needed)

These run without Geode, on any machine with g++ 13 or newer and ffmpeg on the PATH:

```bash
for t in tests/test_*.cpp; do
  n=$(basename "$t" .cpp)
  g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. "$t" -o "/tmp/$n" && "/tmp/$n" || echo "FAILED: $n"
done
```
