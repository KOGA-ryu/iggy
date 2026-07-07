# Builder Task Buckets

This is the **shared** builder handoff queue — repair and cleanup work across all
lanes (creative, render, product-app, and claude-lane/spine), not creative-mode
only. The builder claims and executes any lane; each card is lane-tagged by its
author. Tasks are file-based so the queue is visible in git, durable across
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
- **Spine / core-spine cards** (lane-tagged accordingly) carry their own gate
  discipline (`docs/core_spine_work_rules.md`): they link a ratified Gate-0
  preflight and implement exactly ONE review gate. Execute only the stated gate,
  never collapse gates, and honor the preflight's Do-Not firewall.
