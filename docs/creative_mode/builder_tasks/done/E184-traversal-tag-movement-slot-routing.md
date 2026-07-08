# E184 — TraversalTag Movement Slot Routing

## Status

Ready. This is the next narrow traversal-catalog cleanup after E183.

Commit prefix after review: `claude: planned. codex: ...`.

## Goal

Route movement traversal slot tag parsing and clamber preferred-tag lookups
through the shared `src/content/assets/TraversalTag.hpp` catalog, while
preserving movement slot behavior and display strings.

This should reduce raw movement traversal-tag payload comparisons in
`src/runtime/movement/MovementTraversalSlots.cpp` without touching slot-kind
names, mechanic names, legacy mesh-id fallback strings, or height-band labels.

## Scope

Expected source touch point:

- `src/runtime/movement/MovementTraversalSlots.cpp`

Focused tests may be updated only if a small guard is needed.

Route only these traversal-tag payload uses:

- `kindForTraversalTag(...)` should parse/compare against catalog tags for
  `TraversalTag::Vault`, `TraversalTag::Clamber`, and
  `TraversalTag::WireWalk`.
- clamber top-surface lookup should prefer
  `traversalTagId(TraversalTag::Clamber)`.
- clamber actor-blocker lookup should prefer
  `traversalTagId(TraversalTag::Clamber)`.

Behavior must remain identical:

- authored `vault`, `clamber`, and `wire_walk` tags still build the same slot
  kinds
- unknown tags still do not create affordances
- `clamber_candidate` must not become a movement slot tag
- legacy mesh-id fallbacks still behave exactly as before
- `movementTraversalSlotKindName(...)` output stays byte-identical

## Explicit Non-Scope

Do not migrate or change:

- `movementTraversalSlotKindName(...)` display strings
- `TraversalMechanic` names in `MovementTraversal.cpp`
- `PlayerMotor` phase/display names
- debug/projection display strings
- height-band labels such as `vault_low`, `clamber_mid`, and `wire_balance`
- legacy mesh-id fallback checks such as ids containing `vault`
- room mesh role strings such as `rail`, `ledge`, or `wall`
- ASCII/RoomAsset emitters and validators already handled by E181-E183
- traversal tag catalog values
- collision role stringifiers
- save/load format
- `ProductAppWindowState`
- renderer/Vulkan/window code

If raw `"vault"`, `"clamber"`, or `"wire_walk"` literals remain in
`MovementTraversalSlots.cpp`, classify them in the completion brief. Display
names, height bands, and legacy fallback literals are expected to remain.

## Required Proof

Add or keep focused tests proving:

- authored `wire_walk` still builds a WireWalk slot without relying on mesh-id
  magic
- authored `vault` still suppresses the legacy wire fallback on a rail whose id
  contains both wire/vault hints
- authored clamber still requires the clamber tag on both top and blocker
  surfaces
- `clamber_candidate` or another non-movement catalog tag does not create a
  movement affordance

Do not add a broad new test harness if existing
`movement_traversal_slots_tests` already covers most of the behavior. Add only
the missing guard if needed.

## Acceptance Gates

Run at minimum:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d movement_traversal_slots_tests movement_traversal_tests player_motor_tests traversal_tag_catalog_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(movement_traversal_slots_tests|movement_traversal_tests|player_motor_tests|traversal_tag_catalog_tests)$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

Full CTest is encouraged but not mandatory unless this slice expands beyond the
single movement slot file plus focused tests.

## Required Report

Completion brief must include:

- exact files changed
- which movement slot tag parsing/lookups now use `TraversalTag`
- whether slot construction, legacy fallback, or slot kind display output
  changed
- remaining raw traversal-like literals in `MovementTraversalSlots.cpp` and why
  each remains local
- tests/checks run
- receipt golden result
- confirmation that movement mechanic names, PlayerMotor phase names, debug
  strings, RoomAsset/ASCII serialization, collision role strings,
  ProductAppWindowState, and renderer/Vulkan were not changed

## Completion Brief

- Files changed:
  - `src/runtime/movement/MovementTraversalSlots.cpp`
  - `tests/unit/movement_traversal_slots_tests.cpp`
  - `docs/creative_mode/builder_tasks/ready/E184-traversal-tag-movement-slot-routing.md` moved to `docs/creative_mode/builder_tasks/done/E184-traversal-tag-movement-slot-routing.md`
- Movement slot tag parsing/lookups now using `TraversalTag`:
  - `kindForTraversalTag(...)` now calls `parseTraversalTag(...)` and maps only `TraversalTag::Vault`, `TraversalTag::Clamber`, and `TraversalTag::WireWalk` to movement slot kinds
  - non-movement catalog tags, including `TraversalTag::ClamberCandidate`, return no movement slot kind
  - clamber top-surface preferred lookup now uses `traversalTagId(TraversalTag::Clamber)`
  - clamber actor-blocker preferred lookup now uses `traversalTagId(TraversalTag::Clamber)`
- Slot construction, legacy fallback, and display output:
  - slot construction behavior is preserved
  - legacy mesh-id fallback behavior is preserved
  - `movementTraversalSlotKindName(...)` output is unchanged
  - existing clamber preferred-tag lookup still preserves current fallback semantics; `movement_traversal_tests` pins blocker-tagged clamber with an untagged top surface
- Proof added/kept:
  - existing authored `wire_walk` and `vault` tests still prove authored tags build slots without mesh-id magic and suppress legacy fallback
  - existing clamber tests still prove no authored clamber tag means no clamber affordance/slot
  - added `movement_traversal_slots_tests` coverage proving `clamber_candidate` does not create a movement affordance or slot
- Remaining raw traversal-like literals in `MovementTraversalSlots.cpp`:
  - `hasText(mesh.id, "vault")` remains the legacy mesh-id fallback check
  - `movementTraversalSlotKindName(...)` raw `"vault"`, `"clamber"`, and `"wire_walk"` remain display/output strings
  - `movementTraversalSlotKindName(...)` fallback `"clamber"` remains existing display fallback
  - height-band labels such as `vault_low`, `clamber_mid`, and `wire_balance` remain local movement labels
  - mesh role strings such as `rail`, `ledge`, and `wall` remain local role checks
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d movement_traversal_slots_tests movement_traversal_tests player_motor_tests traversal_tag_catalog_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(movement_traversal_slots_tests|movement_traversal_tests|player_motor_tests|traversal_tag_catalog_tests)$' --output-on-failure`
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
- Receipt golden result:
  - `receipt key-order oracle: 1032 fields match golden (order + values)`
- Scope confirmation:
  - movement mechanic names, `PlayerMotor` phase names, debug strings, `RoomAsset`/ASCII serialization, collision role strings, `ProductAppWindowState`, and renderer/Vulkan were not changed
- Concerns/deferred:
  - full CTest was not run; this stayed within the single movement slot file plus focused tests
  - E184 requested proof wording says clamber requires the tag on both top and blocker surfaces, but the current accepted behavior is blocker-authored clamber with an untagged top fallback; I preserved that behavior because changing it failed `movement_traversal_tests`
