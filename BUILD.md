# Build and test: recorder (PC)

## The commit to test

**`8f0f45f`** on `workflow-claude/recorder`: "Read recorder frames straight from the render target".

This commit has the live level recorder. It fixes the 388800 / 1555200 byte mismatch that produced zero frames on PC: the frame is now read straight from the render target instead of through `newCCImage`.

It does not include:
- the portrait-preview slope fix (`52f46f3` on master);
- the MP4 encoder (`workflow-claude/video-recorder`, commit `e7fecd4`). The recorder still writes PNG frames.

## 1. Requirements (Windows PC)

| Tool | Notes |
|---|---|
| Git | any recent version |
| Geode CLI | must build against SDK 5.11.2 |
| Geode SDK 5.11.2 | check with `geode sdk version` |
| g++ 13 or newer | for the portable tests (MinGW-w64, or WSL) |

Status: none of these steps have been run on your PC yet.

## 2. Check out the commit

```powershell
git clone https://github.com/bagelsssss/GeoDan.git
cd GeoDan
git fetch origin
git checkout 8f0f45f
git log --oneline -1
```

The last line must start with `8f0f45f`.

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

Every test should print a pass line. The dan-analyzer and image-parser tests are for their own branches; if one of them fails here, note it and carry on.

## 4. Build the mod

```powershell
cd tools
geode build --config RelWithDebInfo
```

The `.geode` file is written under `tools\build`. Do not copy it into the game's Mods folder until the build succeeds.

## 5. In-game test

1. In the Tools popup, turn on **Level recorder**.
2. Open a level and play for about 5 seconds, then restart. Each restart starts a new session.
3. Find the session folder:
   ```powershell
   Get-ChildItem -Recurse -Directory -Filter recordings -Path "$env:LOCALAPPDATA\..\..\" -ErrorAction SilentlyContinue
   ```
   The usual location is inside the Steam library under `steamapps\compatdata\322170`. Each session is a `session_<unix ns>` folder.
4. Check the session folder:
   - `manifest.json` has `"status": "complete"` after a normal stop.
   - `frames.csv` has one row per frame.
   - `frames\frame_000000.png` opens and is 360 × 1080.
5. Check the on-screen status line in the portrait preview. It should read `REC queued N` with `drops 0`, and `N` should rise while you play.

Report a failure with the Geode log (search for `GeoDan`), `manifest.json`, and a screenshot of the portrait preview.
