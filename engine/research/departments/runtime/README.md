# Runtime Department

## Charter

Own runtime gameplay execution, command flow, session state, save/load,
orchestrated/profile scenario runners, frame reports, and compatibility
boundaries.

## Roles

- Planner: turns roadmap runtime goals into scoped runtime packets.
- Builder: implements runtime behavior or focused tests.
- Researcher: checks ownership, hot paths, reference patterns, and compatibility
  users.
- Reviewer: gates source-of-truth, save/load risk, public report fields, and
  behavior compatibility.
- Finisher: trims report/count/projection bloat and documents cleanup gates.
- Apprentice/Spark: scans repeated fields, test selectors, and mechanical
  helper candidates.

## Bucket

1. Runtime report/count projection cleanup seam.
2. Scenario/ledger/report duplication split after the current seam lands.
3. Runtime API compatibility debt map for public fields that cannot move yet.
4. Save/load authored-package boundary gate, only after reviewer approval.
5. Legacy `modules/npc_ai` deletion-prerequisite map; no deletion in this lane.

## Hard Stops

- Do not change save formats without a dedicated gate.
- Do not delete `modules/npc_ai`.
- Do not remove public report/result fields while tests or consumers observe
  them.
- Do not change CLI output or authoring facade behavior as runtime cleanup.

## Verification

Focused runtime tests for touched frame/report/session files, then full engine
CTest before integration if production runtime files changed.
