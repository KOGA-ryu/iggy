# Integration Inbox 0001: Real Department Workflow Test

## Objective

Use the local Mac department workflow on real completed branch work, not a
paper-only exercise.

## Branches

1. Runtime cleanup:
   - `/Users/kogaryu/iggy-finisher`
   - `codex/finisher-runtime-cleanup`
   - `bbf45d33 Extract runtime inventory event count projection`

2. AI/NPC region fixture:
   - `/Users/kogaryu/iggy-builder-aimap`
   - `codex/builder-ai-map-regions`
   - `48566a8e Add region AI map fixture coverage`

## Planned Order

1. Request reviewer merge gate.
2. Merge runtime cleanup first if PASS.
3. Merge AI/NPC region fixture second if PASS.
4. Run focused tests for changed surfaces.
5. Run full integration verification.
6. Record the result in `engine/research/departments/integration/decisions/`.

## Hard Stops

- Do not merge if reviewer blocks either branch.
- Do not resolve semantic conflicts silently.
- Do not run both branch integrations in parallel.
- Do not treat tmux windows as worker memory; durable files and commits are the
  source of truth.
