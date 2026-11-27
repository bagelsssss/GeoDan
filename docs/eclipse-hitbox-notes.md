# Eclipse Menu hitboxes: how they work

Source read: `bagelsssss/eclipsemenu-copy` at `770960b` (a copy of EclipseMenu `main`).
Files: `src/hacks/Level/ShowHitboxes.cpp` (345 lines), `src/hacks/Level/HitboxMultiplier.cpp` (139 lines), `src/hacks/Player/ShowTrajectory.cpp` (trajectory, separate notes).

## The core idea: Eclipse does not compute hitboxes

Eclipse never reads the level's objects to find their shapes. It uses the game's own debug draw:

- `GJBaseGameLayer::updateDebugDraw` (the game's hitbox draw) fills `m_debugDrawNode`, a `CCDrawNode`.
- Eclipse hooks `CCDrawNode::drawPolygon` and `drawCircle` and changes the colour and border of each call before it reaches the original.
- So every shape Eclipse shows is one that RobTop's debug draw already produces. Eclipse only restyles it.

Consequence: what appears is whatever the game draws. Objects the game does not draw do not appear, and Eclipse does not have its own list of types.

## How a shape is classified: by colour

`getHitboxType(borderColor)` reads the game's colour:

| Game colour pattern | Eclipse type | Restyle when custom colours are on |
|---|---|---|
| `r == 0` and `b == 1` | Solid | solid colour (blue) |
| `g == 1` | Player | player-rotated colour (yellow), optionally hidden |
| `r == 0` otherwise | Other | other colour (green) |
| anything else | Danger | danger colour (red) |

Eclipse therefore relies on RobTop's colour choices. It does not check object types itself.

## Drawing settings

- Border size: `level.showhitboxes.bordersize`, default 0.25.
- Fill: `fillalpha`, default 0.25, only used when `fillalpha.toggle` is on. Fill colour matches the border colour.
- The fill and border are applied in `customDraw`, before the original call runs.

## Player hitboxes

`drawForPlayer` draws two rectangles per player:

- Outer: `player->getObjectRect()`.
- Inner: `getObjectRect(0.25, 0.25)` when `m_vehicleSize >= 1` (cube-size vehicles such as the ship and ufo), otherwise `getObjectRect(0.4, 0.4)` (mini).

The outer rectangle uses the player colour and the inner one uses the inner colour. Player 2 is drawn only when `m_gameState.m_isDualMode` is true.

## Trail

- Recorded in `GJBaseGameLayer::processCommands`, once per game tick, not per rendered frame. It is off unless `traillength.toggle` is on.
- Stores pairs `(outer rect, inner rect)` in a deque for player 1, and for player 2 in dual mode.
- Trimmed to `traillength` entries (default 240, so 240 ticks, i.e. 4 s at 60 Hz).
- Cleared on `PlayLayer::resetLevel`. Recording stops while `s_isDead` is true.
- Drawn each frame with `drawRect`, using the same colours as the live player.

## Hooks that matter

- `CCDrawNode::drawPolygon` and `drawCircle`: restyle game-drawn shapes. `s_skipDrawHook` stops the hook from re-processing Eclipse's own rectangles.
- `PlayLayer::updateProgressbar`: calls `updateDebugDraw` when hitboxes should show (not while RobTop's own hitboxes are on), then `forceDraw`.
- `PlayLayer::resetLevel`: clears `s_isDead` and the trail.
- `PlayerObject::playerDestroyed`: sets `s_isDead` when player 1 or 2 dies. This drives "Hitboxes On Death".
- `GJBaseGameLayer::processCommands`: records the trail.
- `GJBaseGameLayer::updateDebugDraw`: sets `s_slopeHitboxFix` around the call.
- `GameObject::determineSlopeDirection`: skipped while `s_slopeHitboxFix` is on. This stops slope hitboxes from flipping during the mirror portal transition. The original comment points to a Misc Bugfixes PR.
- `LevelEditorLayer::updateEditor`: `forceDraw` for the editor, controlled by `level.showhitboxes.editor`.

## Other details

- RobTop's own hitboxes (practice mode, game variable `0166`; editor `0045`) are respected. Eclipse does not hide them.
- `isCheating()` returns true when hitboxes are on outside practice mode, unless "Hitboxes On Death" is on. Eclipse treats the feature as cheating outside practice mode.
- `toggleOffHitboxes` restores the game's own `m_debugDrawNode` visibility when hitboxes are turned off.

## What this means for GeoDan

1. **Unclassified objects.** Eclipse can show type 40 if RobTop draws it, because it has no type list. GeoDan's `classifyType` decides that a type is "unclassified" and then marks the whole frame unsupported. Eclipse would show the object in whatever colour the game gives it.
2. **Possible alternative to test.** Read the colour of each shape from the game's own debug draw, the way Eclipse does, instead of relying only on our type table. This would need a check that the shape colours match the classifier for known types first.
3. **Trail and zoom.** Eclipse records the trail on the game tick and draws it in game coordinates. GeoDan captures a separate render target, so its trail and zoom must match the same tick and scale. Matching the on-screen preview depends on this.
4. **Player rectangles.** Eclipse's outer and inner rectangles match GeoDan's `addPlayer` (outer `getObjectRect(0.3, 0.3)`, inner variant). Check the inset against Eclipse's rules: 0.25 for big vehicles and 0.4 for mini, rather than a fixed 0.3.
5. **Slope fix.** GeoDan does not change the slope direction, so the fix is not needed. Check that the sloped hitboxes still match the preview after mirror portals.
