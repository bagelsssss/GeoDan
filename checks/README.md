# Agent Checklist Guide

This directory is GeoDan's shared handoff and coordination record. Claude and Codex must read this guide and **both agent checklists** before starting project work, reviewing commits, or deciding what to implement next.

## Files

- [`claude-check-list.md`](./claude-check-list.md) — Claude's work log and handoff notes.
- [`codex-check-list.md`](./codex-check-list.md) — Codex's work log and handoff notes.
- [`README.md`](./README.md) — this guide and the shared procedure.

Neither checklist replaces the other. Together they provide the agent handoff context.

## Required startup procedure

Both agents must:

1. Read this README.
2. Read **both** `claude-check-list.md` and `codex-check-list.md` in full.
3. Check the current branch, working tree, and latest repository commit. Do not assume a recorded SHA is still current.
4. Compare the checklists for active work, completed work, decisions, constraints, conflicts, and stale or unverified claims.
5. Identify the exact task and files before editing. Avoid conflicting edits when the other agent is working in the same area.
6. Treat checklist entries as handoff notes, not proof that code exists in the current branch.

## While working

Each agent updates **its own checklist only**. Do not silently edit the other agent's log.

Record meaningful progress:
- Current task, stage, and objective.
- Decisions and their source (user instruction, test evidence, code inspection, or proposal).
- Exact files inspected and changed.
- Tests/commands run and exact outcomes; distinguish passed, failed, skipped, and not run.
- Relevant commit SHA(s).
- Unresolved bugs, risks, questions, and blockers.
- Explicit constraints on what must not be changed or referenced.

## Before committing

1. Re-read both checklists for new work or conflicts.
2. Inspect the actual diff and ensure only intended files are included.
3. Run relevant tests and record exact results. Never claim an unrun test passed.
4. Do not overwrite another agent's work or include unrelated changes.
5. Record the resulting commit SHA in your own checklist.

## Required handoff before stopping

Before ending a session, running out of context/tokens, or handing work to the other agent, update your own checklist with:

- Objective and current task.
- Completed and incomplete work.
- Exact files changed and important files inspected.
- Current branch and latest relevant commit SHA, if known.
- Exact tests/commands run and results.
- Precise stopping point.
- Remaining steps in execution order.
- Known problems, uncertainties, and user decisions to preserve.
- A clear next-action instruction for the other agent.

Then read the other agent's checklist once more and ensure the handoff is consistent. Explicitly state when the other agent should take over.

## Reviewing commits

Whenever asked to inspect or review a commit:

1. Read both checklists first.
2. Verify the commit and inspect its actual diff.
3. Compare the diff with both agents' recorded work for ownership, overlap, and regressions.
4. Separate confirmed facts from assumptions and recommendations.
5. Record findings in your own checklist.

## Conflicts and source of truth

- Repository code and diffs determine what is implemented.
- Actual test output determines test status.
- The user determines requirements and project decisions.
- The two checklists document agent tasks and handoffs, subject to repository verification.
- If the checklists disagree, investigate commits, diffs, and test output. Ask the user if an undecided requirement is involved.
- Never mark planned, investigated, or partially implemented work as complete.
- Preserve history; add dated entries and clearly mark superseded notes.

## Ownership

- Claude maintains `claude-check-list.md`.
- Codex maintains `codex-check-list.md`.
- Both agents read both files.
- Each agent may flag a problem in the other checklist but must not silently rewrite the other agent's notes.

## Initial baseline (verify before relying on it)

The last baseline checked while preparing these files was:

- Branch: `main`
- Commit: `48ae322f3b94f5d427f2af9b35aed5a1da7844dd`
- Message: `Hitboxes and hitbox trail: ported logic with tests`

The commit message reports hitbox model/trail tests passing with `g++`; it does **not** prove the complete mod builds or that the features were tested in-game. Re-check the current branch head.

At that baseline, the Tools menu foundation and hitbox/unclassified/trail overlay work were committed. A fresh trajectory implementation was **not** included. Do not call trajectory complete unless a later diff and appropriate tests establish that.

## Non-negotiable project constraints

- Do not modify the archived `GeoDan-deprecated` repository.
- The old trajectory implementation is known to be broken. Do not consult, copy, port, or reuse it as a reference. Build new trajectory work from verified Geometry Dash movement/collision behavior and current Geode bindings, as directed by the user.
- Never claim code, tests, builds, or in-game behavior were verified unless they were actually checked.
- Preserve user-approved architecture and decisions. Label new ideas as proposals until approved.

## Isolated agent workflows and approved integration workflow

The project uses three separate local working directories. These are workflow copies, not three names for the same checkout:

| Directory | Owner / purpose | May contain |
|---|---|---|
| `~/workflow-codex` | Codex's isolated implementation workspace | Codex experiments, drafts, tests, and candidate implementations |
| `~/workflow-claude` | Claude's isolated implementation workspace | Claude experiments, drafts, tests, and candidate implementations |
| `~/main-workflow` | The user-approved integration workspace and source used for the actual GeoDan code | Only implementations the user has selected or explicitly approved for integration |

### Rules for the three workflows

1. Claude works in `~/workflow-claude`; Codex works in `~/workflow-codex`. Neither agent may use the other agent's workspace as its own working directory or overwrite the other agent's files.
2. Each agent may independently implement, test, revise, or discard its own candidate. Experimental work must remain isolated in that agent's workflow.
3. Candidate implementations are not automatically part of GeoDan just because they compile, pass tests, or are committed in an agent workspace.
4. The user decides which candidate (or which specific parts of candidates) should be promoted. For example, if the user prefers Claude's trajectory implementation, the approved Claude changes are copied or merged into `~/main-workflow`; if the user prefers Codex's, the approved Codex changes are promoted instead.
5. Selective combination is allowed only after reviewing the differences and resolving dependencies, conflicts, and incompatible assumptions. Do not blindly combine entire workspaces.
6. Before promotion, the responsible agent must summarize the candidate's changes, exact commit or revision, tests and results, known limitations, and files to be promoted. The user must explicitly approve the promotion.
7. Promotion into `~/main-workflow` must preserve provenance: record which agent/workspace and revision each promoted change came from. Do not silently replace an existing implementation.
8. `~/main-workflow` is the integration/build source of truth for the actual mod. Run integration checks there after promotion. Do not describe an experimental candidate as the active GeoDan implementation until it has been promoted.
9. Keep the three working directories separate. Do not assume they exist, share a Git history, or are synchronized; verify their actual state before operating on them.
10. The GitHub repository and its branches/commits remain the shared remote record. Local workflow directory names do not themselves establish that a change was pushed or committed.

### Required status in agent handoffs

Each agent's checklist entry for active work must identify:
- Which workspace it is using (`~/workflow-codex` or `~/workflow-claude`).
- Its branch and exact HEAD/revision.
- Whether the work is experimental, awaiting review, approved for promotion, or already promoted.
- Exact promotion source and destination when applicable.
- Whether `~/main-workflow` was changed (it must not be changed before user approval).

The user owns the final selection and promotion decision. If approval is unclear, leave the candidate in its isolated workspace and ask before promoting it.


## Branching policy and current trajectory candidate

Agent implementation must happen on isolated candidate branches, never directly on `main`. Each agent uses its own branch and its own local workspace; do not share a candidate branch between agents.

- `main` is the shared baseline and documentation branch.
- Codex's current trajectory candidate branch is `workflow-codex/trajectory-rebuild`.
- Claude must use a separate branch for any independent trajectory candidate.
- Candidate branches are not the integration branch and do not automatically become the active mod.
- The user-approved integration/build source remains `~/main-workflow`. Only explicitly approved changes may be promoted there.
- A remote branch does not create a local directory. Local checkout setup must be verified separately.
- Keep project workflow documentation and checklists on `main` so both agents can read the same rules, while keeping experimental implementation commits on their respective candidate branches.

The Codex trajectory branch was created from `main` at `6a2d5a1ffd2905911ac4f26c8a3829bcfb4b9bfd`. Its checklist kickoff commit is `b381803c0bddf021cb7754f556a57fbd58d96d45`. At this point, it contains workflow documentation only; no trajectory implementation code has been written.
