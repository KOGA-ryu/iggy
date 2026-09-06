# iggy3d Agent Guidance

This is a Creative-only repository. Work directly unless the user explicitly
asks for delegation. Do not spawn subagents for routine audits, cleanup,
implementation, testing, or review.

Before changing a domain, read:

- `docs/departments/README.md`
- `docs/departments/CLEANUP.md`
- the owning department's `DEPARTMENT.md`

## Workstream Rules

- Take one user-visible capability or one semantic concept at a time.
- Trace the live route from user action through its canonical owner to final
  document, render, collision, persistence, or runtime output.
- Do not begin with file size, file splitting, or a broad repository audit.
- Prefer deleting and merging over extracting files.
- A normal cleanup workstream reduces production LOC and does not increase
  production file count.
- Do not add compatibility wrappers, umbrella headers, duplicate helpers, or
  parallel mutation routes without a verified live consumer.
- UI and adapters collect input and present results; they do not independently
  recalculate domain policy.
- Moving code without removing ambiguity, dependencies, declarations, or
  implementation volume is not a completed cleanup.

If a justified ownership repair must increase production LOC or file count,
report that exception before editing and stop for direction.

## Verification

- Derive affected targets from the current build graph; do not reuse stale test
  lists from an older task.
- Use the smallest gate proportional to risk:
  - dead route: affected build target and one surviving-route test;
  - consolidation: boundary test and strongest end-to-end test;
  - semantic correction: one final observable regression test;
  - persistence or play boundary: relevant save/load or runtime integration
    test.
- Run a broader department gate only at a department milestone.
- Do not launch a window or run a broad CTest loop unless requested.
- Do not repeat a passing implementation gate during review unless its evidence
  is inconsistent or the boundary is high risk.

## Checkpoints

Stop after one capability-level workstream. Keep the report compact:

- canonical owner established;
- conflicting routes deleted;
- production files and LOC change;
- verification performed;
- remaining risk;
- next candidate.

Keep changes uncommitted unless the user explicitly asks for a commit. Preserve
unrelated worktree changes.
