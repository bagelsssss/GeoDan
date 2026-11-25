# Codex Checklist

> **Owner:** Codex maintains this file. Claude reads it but must not silently edit it.
>
> **Required:** Before project work or commit review, read [`README.md`](./README.md) and [`claude-check-list.md`](./claude-check-list.md) as well as this file.

## Current status

- **Status:** Awaiting Codex's first entry.
- **Active task:** None recorded.
- **Current stage:** Not initialized.
- **Baseline at checklist creation:** `main` — `8ce48311b51acae5bff35a1e117847b77df70f13` (Claude checklist creation commit; verify current HEAD before relying on it).

## Project constraints

- Do not modify the archived `GeoDan-deprecated` repository.
- The old trajectory implementation is known to be broken. Do not consult, copy, port, or reuse it as a reference.
- Do not claim tests, builds, or in-game verification that were not actually performed.
- Read both agent checklists before project work and commit reviews.
- Update this file with concrete progress and a precise handoff before stopping.

## Active work

- **Status:** Active — isolated candidate, not approved for promotion.
- **Active task:** Rebuild the trajectory tool from verified Geometry Dash behavior and current Geode bindings; do not consult or reuse the old trajectory implementation.
- **Workspace:** `~/workflow-codex` (remote branch prepared; local checkout not yet verified or created).
- **Branch:** `workflow-codex/trajectory-rebuild`
- **Starting HEAD:** `6a2d5a1ffd2905911ac4f26c8a3829bcfb4b9bfd` (the current `main` head when this branch was created).
- **Current stage:** Workspace setup and source inspection. No trajectory source files have been written on this branch.
- **Promotion state:** Experimental; not approved for `~/main-workflow`.
- **Main workflow changed:** No.

## Decisions and findings

- **User decision:** Keep Codex and Claude implementations isolated in `~/workflow-codex` and `~/workflow-claude`; only user-selected changes may be promoted to `~/main-workflow`.
- **Verified repository finding:** `tools/mod.json` already has a disabled-by-default `trajectory` setting.
- **Verified repository finding:** Current gameplay overlay is `tools/src/overlay.cpp`; no trajectory implementation currently exists in the inspected `core/`, `tools/src/`, or `tests/` paths.
- **Constraint:** The old trajectory implementation is explicitly forbidden as a reference. Do not inspect it, copy it, port it, or use it to resolve behavior.
- **Proposal, not yet implemented:** Build a pure core prediction model and tests first, then investigate a side-effect-isolated game simulation layer and rendering.

## Test and build record

| Date | Command / test | Result | Notes |
|---|---|---|---|
| 2026-11-25 | GitHub repository file inspection | Completed | Read README, both checklists, `tools/mod.json`, and `tools/src/overlay.cpp`. No local build or tests run. |

## Files changed

- `checks/codex-check-list.md` — this work-status entry only. No implementation files changed.

## Handoff / next action

- **Last updated:** 2026-11-25.
- **Completed:** Created isolated remote branch `workflow-codex/trajectory-rebuild` from current `main`; inspected current integration points.
- **Remaining:** Verify/create local `~/workflow-codex` checkout; inspect current core/test/build layout and Geode bindings without consulting the old trajectory code; design and implement the pure model; add tests; only then investigate game simulation.
- **Exact stopping point:** Remote branch exists. Local workspace path and current local checkout states are unknown.
- **Next action:** Ask the user to run a read-only local workspace/branch status command before adding or modifying worktrees.
- **Blockers / risks:** Cannot directly inspect or create directories on the user's CachyOS filesystem from this session. Hidden-player updates/collision calls may have global side effects and must not be assumed safe.
- **Relevant commit:** Branch starts at `6a2d5a1ffd2905911ac4f26c8a3829bcfb4b9bfd`; checklist update commit will follow.

## Session log

### 2026-11-25 — trajectory rebuild started
- Read the shared README and both agent checklists.
- Verified current `main` head and inspected trajectory setting and gameplay overlay.
- Created isolated Codex candidate branch; no implementation code or main-workflow files changed.

## Session log

_Add newest entries first. Include date/time and relevant commit SHA._
