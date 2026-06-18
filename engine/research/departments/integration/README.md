# Integration Department

## Charter

Own branch health, merge order, reviewer gates, conflict preflight, final
verification, and roadmap/bucket sync after work lands on `master`.

## Planner Job

The integration planner does not build features. It decides what can merge,
what must wait, which reviewer gate is required, and what verification proves
the integrated state is sound.

## On-Demand Workers

- Reviewer: read-only merge gates, conflict risk, changed-file hotspots.
- Researcher: branch topology, ownership map, stale-doc scans.
- Builder: only for integration scripts or mechanical merge helpers.
- Finisher: roadmap/bucket sync after merges.
- Apprentice/Spark: status scans and focused changed-file inventory.

## Bucket

1. Gate completed runtime and AI/NPC branches from the real workflow test.
2. Merge only after reviewer PASS.
3. Run focused tests tied to changed files.
4. Run the full integration gate.
5. Record decisions in `decisions/`.
6. Sync roadmap or bucket docs only if the merge changes project state.

## Hard Stops

- Do not resolve semantic conflicts without planner/user review.
- Do not force-update shared branches.
- Do not merge branches that alter CLI output, save/load formats, or public
  result shapes without explicit reviewer gate coverage.
- Do not let docs-only queue cleanup hide failed code verification.

## Verification

Before merge: `git diff --name-status master...branch` and reviewer gate.

After merge: focused tests for touched files, then:

```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
```
