# Build and test: MP4 encoder commit (PC)

## The commit to test

**`e7fecd4`** on `workflow-claude/video-recorder`: "Record the level as an MP4 through ffmpeg".

This commit adds the ffmpeg encoder and the MP4 storage mode in the portable core. The in-game recorder is **not** wired to it yet, so the mod does not record MP4 in game. The in-game test checks the master features only.

## 1. Requirements (Windows PC)

| Tool | Notes |
|---|---|
| Git | any recent version |
| Geode CLI | must build against SDK 5.11.2 |
| Geode SDK 5.11.2 | check with `geode sdk version` |
| g++ 13 or newer | for the portable tests (MinGW-w64, or WSL) |
| ffmpeg and ffprobe | on PATH; the MP4 tests run them for real |

Status: none of these steps have been run on your PC yet.

## 2. Check out the commit

```powershell
git clone https://github.com/bagelsssss/GeoDan.git
cd GeoDan
git fetch origin
git checkout e7fecd4
git log --oneline -1
```

The last line must start with `e7fecd4`.

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

Every test should print a pass line. `test_recorder_video_sink` and `test_recorder_video_worker` encode a real MP4 and check it with ffprobe. Both need ffmpeg on PATH. Without it, they print a skip line.

## 4. Build the mod

```powershell
cd tools
geode build --config RelWithDebInfo
```

The `.geode` file is written under `tools\build`. Do not copy it into the game's Mods folder until the build succeeds.

## 5. In-game test (master features)

Turn on in the Tools popup: Hitboxes, Unclassified, Hitbox trail. Enable the portrait preview test in the mod settings.

1. A level shows hitbox outlines, and the trail follows the player.
2. The TOOLS button appears on the pause menu and on the level page.
3. The portrait preview shows hitboxes, both ground lines, and the trail, with a yellow `PV ok <count>` line at the bottom left.

Report a failure with the Geode log (search for `GeoDan`) and a screenshot.
