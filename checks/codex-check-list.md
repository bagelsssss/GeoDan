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

- **Status:** Active — isolated trajectory candidate; not approved for promotion.
- **Task:** Fresh trajectory rebuild. The old trajectory implementation must not be consulted, copied, ported, or reused.
- **Remote branch:** `workflow-codex/trajectory-rebuild`
- **Branch base:** `main` at `6a2d5a1ffd2905911ac4f26c8a3829bcfb4b9bfd`
- **Candidate kickoff commit:** `b381803c0bddf021cb7754f556a57fbd58d96d45`
- **Local workspace:** `~/workflow-codex` not yet verified or created; user currently has no access to their Arch PC.
- **Promotion status:** Experimental only. No approval to promote into `~/main-workflow`.
- **Main-workflow changed:** No.
- **Implementation status:** No trajectory implementation code written yet; branch currently records kickoff/workflow notes only.

## Decisions and findings

_Add dated, evidence-backed entries. Label each as a user decision, verified finding, or proposal._

## Test and build record

| Date | Command / test | Result | Notes |
|---|---|---|---|
| — | — | Not recorded | Do not infer test results. |

## Files changed

_Record exact repository-relative paths and a short description._

## Handoff / next action

- **Last updated:** Not initialized.
- **Completed:** No Codex work recorded yet.
- **Remaining:** Populate this section when work begins and before handing off.
- **Exact stopping point:** Not recorded.
- **Next action for Claude:** Read this file and the Claude checklist; coordinate before taking over overlapping work.
- **Blockers / risks:** None recorded.
- **Relevant commit:** `8ce48311b51acae5bff35a1e117847b77df70f13` (baseline at creation; verify current HEAD).

## Session log

### 2026-11-25 — isolated trajectory branch created
- Created `workflow-codex/trajectory-rebuild` from current `main` at `6a2d5a1ffd2905911ac4f26c8a3829bcfb4b9bfd`.
- Added kickoff status to this checklist on the candidate branch (commit `b381803c0bddf021cb7754f556a57fbd58d96d45`).
- User requested that branch workflow and current branch status also be documented in both repository READMEs and in the checks on `main`.
- The root README, `checks/README.md`, and this checklist are shared workflow documentation on `main`. No trajectory code or `~/main-workflow` changes.
- User is away from their Arch PC; local workspace setup is deferred.

