# Builder Task Buckets

This directory is the handoff queue for Creative-mode repair and cleanup work.
Tasks are file-based so the queue is visible in git, durable across Codex
threads, and easy to review.

## Buckets

- `ready/`: tasks available for builder to claim.
- `claimed/`: one task currently owned by builder.
- `done/`: completed tasks with a completion brief appended.
- `blocked/`: tasks builder could not complete cleanly, with blocker notes appended.

## Claim Rules

1. Check `PRIORITY.md` first. If it lists a ready task under Pull Next, claim
   the first ready task in that list.
2. If no priority-listed task is ready, pick the lowest-numbered task in
   `ready/` unless instructed otherwise.
3. Move exactly one task file from `ready/` to `claimed/`.
4. Do the task without widening scope.
5. On completion, append a completion brief to the task file and move it to
   `done/`.
6. If blocked, append the blocker, evidence, and recommended next move, then
   move the file to `blocked/`.

## Completion Brief Format

Append this section to the bottom of the task file:

```md
## Completion Brief

- Files changed:
- Behavior changed:
- Tests/checks run:
- Evidence:
- Concerns/deferred:
```

## Standing Rules

- No staging, commit, push, docs outside this queue, broad CTest, or window
  launch unless the task explicitly asks for it.
- Prefer descriptor/document/kernel truth over app-local object-kind branches.
- Add tests that prove behavior, not only receipt-copy plumbing.
- If a task exposes a broader architectural blocker, stop at the blocker and
  move the card to `blocked/` with exact file/line evidence.
