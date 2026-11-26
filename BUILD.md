# Build and test GeoDan Tools

Test from `workflow-claude/master`. Only finished work lands there, so its head is the build you should test.

## 1. Requirements (Windows PC)

| Tool | Notes |
|---|---|
| Git | any recent version |
| Geode CLI | must build against SDK 5.11.2 (the mod targets it in `tools/mod.json`) |
| Geode SDK 5.11.2 | check with `geode sdk version` |
| g++ 13 or newer | for the portable tests (MinGW-w64, or WSL) |
| ffmpeg and ffprobe | on PATH, for the MP4 test only; the test is skipped without ffmpeg |

Status: none of these steps have been run on your PC yet.

## 2. Get the branch

```powershell
git clone https://github.com/bagelsssss/GeoDan.git
cd GeoDan
git fetch origin
git checkout workflow-claude/master
git log --oneline -1
```

The last line should show `52f46f3` or a newer commit on master.

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

Every test should print a pass line. `test_recorder_video_worker` encodes a real MP4 with ffmpeg and checks it with ffprobe.

## 4. Build the mod

```powershell
cd tools
geode build --config RelWithDebInfo
```

The `.geode` file is written under `tools\build`. Do not copy it into the game's Mods folder until the build succeeds.

## 5. In-game test (master)

Turn on these settings in the Tools popup:

- Hitboxes, Unclassified, Hitbox trail
- Portrait preview test (diagnostic, in the mod settings)

Checks:

1. A level shows hitbox outlines, and the trail follows the player.
2. The TOOLS button appears on the pause menu and on the level page.
3. The portrait preview shows hitboxes, both ground lines, and the trail, with a yellow `PV ok <count>` line at the bottom left.

Report a failure with the Geode log (search for `GeoDan`) and a screenshot.

## 6. What the recorder test covers

The recorder is not in master's build yet. Its status on master is "Planned". Two branches exist, and neither is ready for an in-game MP4 test:

| Branch | Recorder output | In-game test |
|---|---|---|
| `workflow-claude/recorder` | PNG frames | possible, but based on an older master |
| `workflow-claude/video-recorder` | MP4 in the core library only | not possible yet; the live recorder is not wired to it |

Once the MP4 is wired into the live recorder, the in-game check is: a `video.mp4` plays in a video player, and `manifest.json` has `"schema_version": 2` with `"status": "complete"`.
