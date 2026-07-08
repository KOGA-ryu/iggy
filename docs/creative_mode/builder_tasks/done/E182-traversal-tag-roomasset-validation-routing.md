# E182 — TraversalTag RoomAsset Validation Routing

## Status

Ready. This is the next narrow traversal-catalog cleanup after E181.

Commit prefix after review: `claude: planned. codex: ...`.

## Goal

Route RoomAsset traversal-tag validation invariants through the shared
`src/content/assets/TraversalTag.hpp` catalog, while preserving exact RoomAsset
file-format behavior and error reasons.

This should reduce remaining raw traversal-tag payload checks in
`src/content/assets/RoomAsset.cpp` without migrating role parser/display strings
that merely share the same bytes.

## Scope

Expected source touch point:

- `src/content/assets/RoomAsset.cpp`

Focused tests may be updated only if a small guard is needed.

Route only the traversal-tag payload invariant checks:

- Walkable surfaces require traversal tag `TraversalTag::Walkable`.
- Blocker surfaces require traversal tag `TraversalTag::Blocker`.
- ProjectileBlocker surfaces require traversal tag
  `TraversalTag::ProjectileBlocker`.
- Opening surfaces require traversal tag `TraversalTag::Opening`.

The validation behavior must remain identical:

- unknown traversal tags still produce `room_unknown_traversal_tag`
- missing/incompatible required role tags still produce
  `room_invalid_spatial_surface` or `room_invalid_opening_surface`
  exactly as before

## Explicit Non-Scope

Do not migrate or change:

- `parseRole(...)` string comparisons for RoomSpatialSurfaceRole
- `parseShape(...)` string comparisons
- role/stringifier/display helpers
- serializer text shape
- traversal tag catalog values
- movement slot parse tables
- collision role stringifiers
- ASCII/Creative emitters already handled by E181
- save/load semantics beyond the existing RoomAsset validation path
- `ProductAppWindowState`
- renderer/Vulkan/window code

If a raw `"walkable"`, `"blocker"`, `"projectile_blocker"`, or `"opening"`
literal remains in `RoomAsset.cpp`, classify it in the completion brief. Role
parser/file-format literals are expected to remain.

## Required Proof

Add or keep focused tests proving:

- valid RoomAsset surfaces with required traversal tags still load/validate
- a role surface missing its required traversal tag still fails with the same
  reason as before
- an unknown traversal tag still fails with
  `room_unknown_traversal_tag`

Do not add a broad new test harness if existing RoomAsset tests already cover
the cases. If no new tests are needed, cite the existing tests that cover the
invariants.

## Acceptance Gates

Run at minimum:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d room_asset_loader_tests ascii_room_asset_text_tests ascii_room_to_room_asset_tests traversal_tag_catalog_tests product_ascii_room_activation_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(room_asset_loader_tests|ascii_room_asset_text_tests|ascii_room_to_room_asset_tests|traversal_tag_catalog_tests|product_ascii_room_activation_tests)$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

Full CTest is encouraged but not mandatory unless this slice expands beyond the
single validation file plus focused tests.

## Required Report

Completion brief must include:

- exact files changed
- which RoomAsset invariant checks now use `TraversalTag`
- remaining raw traversal-like literals in `RoomAsset.cpp` and why each remains
  local
- tests/checks run
- receipt golden result
- confirmation that parser role strings, serializer format, movement parsing,
  collision role strings, ProductAppWindowState, and renderer/Vulkan were not
  changed

## Completion Brief

- Files changed:
  - `src/content/assets/RoomAsset.cpp`
  - `tests/unit/room_asset_loader_tests.cpp`
  - `docs/creative_mode/builder_tasks/ready/E182-traversal-tag-roomasset-validation-routing.md` moved to `docs/creative_mode/builder_tasks/done/E182-traversal-tag-roomasset-validation-routing.md`
- RoomAsset invariant checks routed through `TraversalTag`:
  - walkable surface validation now checks `hasTraversalTag(surface, TraversalTag::Walkable)`
  - blocker surface validation now checks `hasTraversalTag(surface, TraversalTag::Blocker)`
  - projectile blocker surface validation now checks `hasTraversalTag(surface, TraversalTag::ProjectileBlocker)`
  - opening surface validation now checks `hasTraversalTag(surface, TraversalTag::Opening)`
  - unknown traversal tag validation still uses `validTraversalTag(tag)` and keeps `room_unknown_traversal_tag`
- Tests added/kept:
  - `room_asset_loader_tests` now covers missing required traversal tags for walkable, blocker, projectile blocker, and opening surfaces with the existing deterministic rejection reasons
  - existing valid load/round-trip and unknown traversal tag coverage remains in place
- Remaining raw traversal-like literals in `RoomAsset.cpp`:
  - `parseShape(...)`: `"opening"` remains a file-format shape parser literal
  - `parseRole(...)`: `"walkable"`, `"blocker"`, `"projectile_blocker"`, and `"opening"` remain file-format role parser literals
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d room_asset_loader_tests ascii_room_asset_text_tests ascii_room_to_room_asset_tests traversal_tag_catalog_tests product_ascii_room_activation_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(room_asset_loader_tests|ascii_room_asset_text_tests|ascii_room_to_room_asset_tests|traversal_tag_catalog_tests|product_ascii_room_activation_tests)$' --output-on-failure`
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
- Receipt golden result:
  - `receipt key-order oracle: 1032 fields match golden (order + values)`
- Full CTest result:
  - `100% tests passed, 0 tests failed out of 260`
- Scope confirmation:
  - parser role strings, serializer format, movement parsing, collision role strings, `ProductAppWindowState`, and renderer/Vulkan were not changed
- Concerns/deferred:
  - none
