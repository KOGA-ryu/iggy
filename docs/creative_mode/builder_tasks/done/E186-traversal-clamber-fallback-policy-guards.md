# E186 — Traversal Clamber Fallback Policy Guard Tests

## Status

Ready. This is the behavior-preserving follow-up to the E185 audit.

Commit prefix after review: `claude: planned. codex: ...`.

## Goal

Pin the current clamber authored-tag policy explicitly so future cleanup does
not accidentally tighten behavior.

E185 concluded that the current policy should be preserved: durable `clamber`
authorizes a movement affordance on any same-mesh surface, while measured
walkable top and actor-blocker geometry may fall back to same-mesh helper
surfaces when the preferred tagged surface is absent.

## Scope

Expected source/test touch points:

- `src/runtime/movement/MovementTraversalSlots.cpp`
- `tests/unit/movement_traversal_slots_tests.cpp`

Implement only behavior-preserving tests and a short clarifying comment.

Add focused guards for:

- top-only durable `clamber`:
  - walkable top has `clamber`
  - same-mesh actor blocker lacks `clamber`
  - registry creates one authored clamber affordance and one clamber slot
  - slot uses blocker fallback geometry
  - affordance source is the top surface
- blocker-only durable `clamber`:
  - walkable top lacks `clamber`
  - same-mesh actor blocker has `clamber`
  - registry creates one authored clamber affordance and one clamber slot
  - slot uses top fallback geometry
  - affordance source is the blocker surface

Keep existing guards for:

- both top and blocker tagged
- neither top nor blocker tagged
- `clamber_candidate` without durable `clamber`
- legacy rail `vault`/`wire` fallback behavior

Add a short comment near `appendClamberSlot(...)` or the helper lookup seam
explaining the policy from E185:

- `clamber` authorizes the affordance on the mesh
- top/blocker surfaces are measured helper geometry
- preferred tagged top/blocker surfaces are used when present
- same-mesh helper surfaces are valid fallbacks

## Explicit Non-Scope

Do not change:

- traversal tag catalog values
- slot construction semantics
- clamber fallback behavior
- legacy mesh-id fallback behavior
- RoomAsset/ASCII serialization
- Creative emitters
- movement mechanic display names
- `movementTraversalSlotKindName(...)` output
- `PlayerMotor` phase names
- debug/projection display strings
- collision role strings
- `ProductAppWindowState`
- renderer/Vulkan/window code

This is not a behavior-change card. If a proposed edit changes any existing
movement traversal expectation, stop and move the card to `blocked/` with the
failing test and proposed decision.

## Acceptance Gates

Run at minimum:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d movement_traversal_slots_tests movement_traversal_tests traversal_tag_catalog_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(movement_traversal_slots_tests|movement_traversal_tests|traversal_tag_catalog_tests)$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

Full CTest is encouraged but not mandatory unless the slice expands beyond the
two expected files.

## Required Report

Completion brief must include:

- exact files changed
- the top-only and blocker-only fallback cases added
- confirmation that behavior stayed unchanged
- whether `movementTraversalSlotKindName(...)`, legacy fallbacks, RoomAsset/ASCII
  serialization, traversal catalog values, ProductAppWindowState, and
  renderer/Vulkan were untouched
- tests/checks run
- receipt golden result

## Completion Brief

- Files changed:
  - `src/runtime/movement/MovementTraversalSlots.cpp`
  - `tests/unit/movement_traversal_slots_tests.cpp`
  - `docs/creative_mode/builder_tasks/ready/E186-traversal-clamber-fallback-policy-guards.md` moved to `docs/creative_mode/builder_tasks/done/E186-traversal-clamber-fallback-policy-guards.md`
- Top-only fallback guard added:
  - `topOnlyClamberUsesBlockerFallbackGeometry()` builds a room where the walkable top has durable `clamber` and the same-mesh actor blocker lacks it
  - proves one authored clamber affordance and one clamber slot are created
  - proves the affordance source is the top surface
  - proves the slot uses the untagged blocker as fallback geometry
- Blocker-only fallback guard added:
  - `blockerOnlyClamberUsesTopFallbackGeometry()` builds a room where the walkable top lacks durable `clamber` and the same-mesh actor blocker has it
  - proves one authored clamber affordance and one clamber slot are created
  - proves the affordance source is the blocker surface
  - proves the slot uses the untagged top as fallback geometry
- Clarifying comment added:
  - near `appendClamberSlot(...)`, documenting that durable `clamber` authorizes the mesh affordance, while top/blocker surfaces are measured helper geometry that prefer tagged surfaces but allow same-mesh untagged fallbacks
- Behavior confirmation:
  - no movement traversal behavior changed
  - existing `movement_traversal_tests` stayed green, including the blocker-tagged clamber with untagged top fallback path
- Untouched scope:
  - `movementTraversalSlotKindName(...)` output unchanged
  - legacy rail `vault`/`wire` fallbacks unchanged
  - `RoomAsset`/ASCII serialization unchanged
  - traversal catalog values unchanged
  - `ProductAppWindowState` unchanged
  - renderer/Vulkan unchanged
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d movement_traversal_slots_tests movement_traversal_tests traversal_tag_catalog_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(movement_traversal_slots_tests|movement_traversal_tests|traversal_tag_catalog_tests)$' --output-on-failure`
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
- Receipt golden result:
  - `receipt key-order oracle: 1032 fields match golden (order + values)`
- Concerns/deferred:
  - full CTest was not run; this stayed within the two expected files plus focused tests
