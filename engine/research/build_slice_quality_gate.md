# Build Slice Quality Gate

Purpose: reusable checklist for planner handoffs before Reviewer Dex forwards work to Builder Dex.

Use this on every non-trivial slice. Keep answers concrete and tied to current code.

## Required Handoff Block

```text
<quality_gate>
  <owner>
    Which subsystem owns this slice, and why?
  </owner>
  <source_of_truth>
    Existing type/helper/module this must reuse instead of duplicating.
  </source_of_truth>
  <must_not_touch>
    Files/subsystems that are out of scope.
  </must_not_touch>
  <defer>
    Features that are tempting but explicitly not part of this slice.
  </defer>
  <default_behavior>
    Existing behavior that must remain unchanged.
  </default_behavior>
  <risk_to_watch>
    What would make this slice too broad, badly placed, or messy?
  </risk_to_watch>
</quality_gate>
```

## Review Questions

Before forwarding, answer:

- What subsystem owns the source data?
- Is the output authoritative state, derived cache, command, query result, or resource metadata?
- Which existing builder/query/helper is the source of truth?
- Does this create a new lifetime owner?
- Does this change tick order, cache rebuild policy, or save semantics?
- Can it be behavior-tested without backend services?

## Stop Conditions

Pause for ownership review when a slice would:

- move authoritative state between subsystems
- make runtime own a derived cache rebuild policy
- introduce save/load semantics
- combine gameplay tick with presentation/render
- remove or add a compatibility bridge
- define mutation semantics for level, player, inventory, or resources
- add a new global service or registry

## Builder Brief Requirements

Builder completion briefs should include:

- `PASS`
- `FILES`
- `PUBLIC API`
- `OWNERSHIP NOTES`
- `BEHAVIOR`
- `VERIFY`
- `NEXT RISK`
- `BLOCKED`

## Verification Baseline

Unless the slice is docs-only:

```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
git ls-files '*Devilution*' '*devilution*' '*DevilutionX*' '*master.zip' '*godot*'
git ls-files --others --exclude-standard '*Devilution*' '*devilution*' '*DevilutionX*' '*master.zip' '*godot*'
```

Docs-only slices should at least run:

```sh
git diff --check
git ls-files '*Devilution*' '*devilution*' '*DevilutionX*' '*master.zip' '*godot*'
git ls-files --others --exclude-standard '*Devilution*' '*devilution*' '*DevilutionX*' '*master.zip' '*godot*'
```

