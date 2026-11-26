# Build and test: recorder with MP4 output (PC)

## The commit to test

**`02345b7`** on `workflow-claude/recorder-video`.

This commit is the recorder branch with master merged in (so it has the slope fix and later preview work) and the MP4 encoder wired in. Each recording session writes one `video.mp4`, not a PNG per frame.

Not yet on master. Master will get it only after you test it.

## 1. Requirements (Windows PC)

| Tool | Notes |
|---|---|
| Git | any recent version |
| Geode CLI | must build against SDK 5.11.2 |
| Geode SDK 5.11.2 | check with `geode sdk version` |
| g++ 13 or newer | for the portable tests (MinGW-w64, or WSL) |
| ffmpeg and ffprobe | on PATH. The recorder runs `ffmpeg`, so it must be found by that name |

Status: none of these steps have been run on your PC yet.

## 2. Check out the commit

```powershell
git clone https://github.com/bagelsssss/GeoDan.git
cd GeoDan
git fetch origin
git checkout 02345b7
git log --oneline -1
```

The last line must start with `02345b7`.

## 3. Portable tests (no game needed)

```powershell
Get-ChildItem tests -Filter test_*.cpp | ForEach-Object {
    $name = $_.BaseName
    g++ -std=c++20 -Wall -Wextra -Werror -pthread -Icore -I. $_.FullName -o "$env:TEMP\$name.exe"
    if ($LASTEXITCODE -ne 0) { throw "compile failed: $name" }
    & "$env:TEMP\$name.exe"
    if ($LASTEXITCODE -ne 0) { throw "run failed: $name" }
}
```

Every test should print a pass line. `test_recorder_video_sink` and `test_recorder_video_worker` run ffmpeg and ffprobe for real.

## 4. Build the mod

```powershell
cd tools
geode build --config RelWithDebInfo
```

The `.geode` file is written under `tools\build`. Do not copy it into the game's Mods folder until the build succeeds.

## 5. In-game test

1. In the Tools popup, turn on **Level recorder**.
2. Open a level, play for about 5 seconds, then restart. Each restart starts a new session.
3. Find the session folder `recordings\session_<unix ns>` in the mod's save directory.
4. Check the session folder:
   - `video.mp4` exists and plays in a video player.
   - `manifest.json` has `"schema_version": 2` and `"status": "complete"` after a normal stop.
   - `frames.csv` has one row per frame.
5. Check the on-screen status line in the portrait preview. It should read `REC queued N` with `drops 0`, and `N` should rise while you play.

Report a failure with the Geode log (search for `GeoDan`), `manifest.json`, and a screenshot of the portrait preview.
