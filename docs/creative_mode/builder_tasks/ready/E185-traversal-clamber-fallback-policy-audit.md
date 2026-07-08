# E185 — Traversal Clamber Fallback Policy Audit

## Status

Ready. Read-only audit prompted by E184.

Commit prefix after review: `claude: planned. codex: ...`.

## Goal

Map the current clamber authored-tag policy before any implementation card tries
to tighten or simplify it.

E184 intentionally preserved behavior after discovering that the current runtime
accepts blocker-authored clamber with an untagged walkable top fallback. My E184
proof wording incorrectly implied both top and blocker surfaces must carry the
`clamber` tag. This audit should make the actual policy explicit and decide
whether the next card is a behavior-preserving documentation/test cleanup or a
deliberate behavior change.

## Scope

Audit only. Do not edit source, tests, CMake, receipt golden, or production
docs beyond this task card.

Inspect at minimum:

- `src/runtime/movement/MovementTraversalSlots.cpp`
- `src/runtime/movement/MovementTraversal.cpp`
- `tests/unit/movement_traversal_slots_tests.cpp`
- `tests/unit/movement_traversal_tests.cpp`
- `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.cpp`
- `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp`
- `src/app/iggy3d/world/MovementTestLab.cpp`

## Questions To Answer

Document the current behavior for each case:

- top surface tagged `clamber`, blocker surface tagged `clamber`
- top tagged `clamber`, blocker untagged
- top untagged, blocker tagged `clamber`
- neither top nor blocker tagged `clamber`
- `clamber_candidate` present but no durable `clamber` tag
- legacy mesh-id fallback paths involving `clamber`

For each case, report whether the current code:

- creates an authored affordance
- creates a clamber slot
- uses top fallback or blocker fallback
- records the affordance source surface as top, blocker, or legacy fallback
- is already pinned by a focused test

## Policy Decision Needed

Recommend one of these paths, with evidence:

1. **Preserve current behavior.** Treat a blocker-authored `clamber` tag as
   enough to authorize clamber, and allow walkable top fallback.
2. **Tighten behavior intentionally.** Require both the walkable top and actor
   blocker surfaces to carry durable `clamber`, with tests and migration notes.
3. **Split semantics.** Define one tag as the authored affordance source and
   another as the measured geometry helper, if current data implies that.

Do not implement the decision in this audit card. Draft the follow-up card in
the completion brief only.

## Explicit Non-Scope

Do not migrate or change:

- traversal tag catalog values
- movement slot construction
- legacy fallback behavior
- RoomAsset/ASCII serialization
- Creative emitters
- movement mechanic display names
- PlayerMotor phase names
- debug/projection display strings
- collision role strings
- `ProductAppWindowState`
- renderer/Vulkan/window code

## Required Commands

Run and report:

```sh
rg -n "clamber|clamber_candidate|legacy_name_fallback|kindForTraversalTag|findSurfaceForMesh|findActorBlockerForMesh" /Users/kogaryu/iggy3d/src/runtime/movement /Users/kogaryu/iggy3d/tests/unit/movement_traversal*_tests.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/ascii_room /Users/kogaryu/iggy3d/src/app/iggy3d/world/MovementTestLab.cpp
git -C /Users/kogaryu/iggy3d diff --check
```

No build or CTest is required unless you add a focused guard test, which this
card does not ask for.

## Required Report

Completion brief must include:

- exact files inspected
- the case matrix from **Questions To Answer**
- tests that currently pin each case, and gaps
- recommendation among the three policy paths
- draft follow-up card title/scope
- confirmation that no source/test/CMake/receipt golden files were edited
