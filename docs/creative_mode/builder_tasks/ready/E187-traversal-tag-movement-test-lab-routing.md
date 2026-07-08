# E187 — TraversalTag Movement Test Lab Routing

## Status

Ready. This is the next narrow traversal-catalog cleanup after E186.

Commit prefix after review: `claude: planned. codex: ...`.

## Goal

Route the durable `clamber` traversal payload emitted by the product movement
test lab through `src/content/assets/TraversalTag.hpp`, while preserving the
existing authored room output and keeping movement-lab gameplay/material labels
local.

The remaining literal in `src/app/iggy3d/world/MovementTestLab.cpp` is part of
authored object traversal tags for ledge objects. It is not a display string or
parser role name, so it should use the shared catalog.

## Scope

Expected source touch point:

- `src/app/iggy3d/world/MovementTestLab.cpp`

Focused tests may be updated only if needed:

- movement test lab / ASCII package / movement traversal tests that already
  cover the generated lab data

Route only the durable traversal payload:

- `MovementLabObjectKind::Ledge` should still emit traversal tags containing
  `ledge` and durable `clamber`, but the durable `clamber` string should come
  from `traversalTagId(TraversalTag::Clamber)`.

Preserve current output:

- generated authored objects still have the same traversal tag strings
- generated gameplay tags stay byte-identical
- material/gameplay labels such as `marker`, `platform`, `wall_run`,
  `collision_slide`, `snag`, `crate`, `ledge`, and `movement_lab` remain local
  movement-lab vocabulary

## Explicit Non-Scope

Do not migrate or change:

- movement slot construction or clamber fallback policy
- `MovementTraversalSlots.cpp` guarded by E184-E186
- `AsciiRoomToAuthoredRoom.cpp` gameplay tag copy
- RoomAsset/ASCII serialization
- traversal tag catalog values
- movement mechanic display names
- `movementTraversalSlotKindName(...)` output
- PlayerMotor phase names
- debug/projection display strings
- collision role strings
- `ProductAppWindowState`
- renderer/Vulkan/window code

If raw `"clamber"` remains in `MovementTestLab.cpp`, classify it in the
completion brief. It should remain only if it is a local gameplay/display label,
not a traversal payload.

## Required Proof

Add or keep focused tests proving:

- generated movement test lab ledge objects still include durable `clamber` in
  authored object traversal tags
- generated gameplay tags remain byte-identical
- package or movement traversal smoke behavior is unchanged if the existing
  test coverage already exercises this path

Do not add a broad new harness if existing movement test lab or package tests
can cover the behavior.

## Acceptance Gates

Run at minimum:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_ascii_package_smoke product_gameplay_controls_smoke movement_traversal_slots_tests movement_traversal_tests traversal_tag_catalog_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_ascii_package_smoke|product_gameplay_controls_smoke|movement_traversal_slots_tests|movement_traversal_tests|traversal_tag_catalog_tests)$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

Full CTest is encouraged but not mandatory unless this slice expands beyond the
single movement test lab file plus focused tests.

## Required Report

Completion brief must include:

- exact files changed
- how MovementTestLab now obtains the durable `clamber` traversal payload
- confirmation that generated traversal/gameplay tag output did not change
- remaining raw traversal-like literals in `MovementTestLab.cpp` and why each
  remains local
- tests/checks run
- receipt golden result
- confirmation that movement slot construction, clamber fallback policy,
  RoomAsset/ASCII serialization, traversal catalog values, ProductAppWindowState,
  and renderer/Vulkan were not changed
