# E183 — TraversalTag ASCII Asset Text Export Routing

## Status

Ready. This is the next narrow traversal-catalog cleanup after E182.

Commit prefix after review: `claude: planned. codex: ...`.

## Goal

Route ASCII RoomAsset text traversal-tag export ordering through the shared
`src/content/assets/TraversalTag.hpp` catalog, while preserving exact serialized
text shape and role/parser behavior.

This should reduce the remaining raw traversal-tag payload literals in
`src/app/iggy3d/ascii_room/AsciiRoomAssetText.cpp` without migrating role names
or shape names that are file-format strings.

## Scope

Expected source touch point:

- `src/app/iggy3d/ascii_room/AsciiRoomAssetText.cpp`

Focused tests may be updated only if a small guard is needed.

Route only the traversal-tag payload insertion inside
`traversalTagsForExport(...)`:

- Walkable role inserts `TraversalTag::Walkable`.
- Blocker role inserts `TraversalTag::Blocker`.
- ProjectileBlocker role inserts `TraversalTag::ProjectileBlocker`.
- Opening role inserts `TraversalTag::Opening`.

Keep the export order byte-identical: the role-derived required tag still comes
first, followed by valid input traversal tags in input order, with duplicates
deduped exactly as today.

## Explicit Non-Scope

Do not migrate or change:

- `shapeName(...)` string output
- `roleName(...)` string output
- serialized TOML/text field names or formatting
- parser role/shape strings in other files
- traversal tag catalog values
- movement slot parse tables
- collision role stringifiers
- RoomAsset validation already handled by E182
- Creative/ASCII emitters already handled by E181
- save/load semantics beyond the existing ASCII asset text export path
- `ProductAppWindowState`
- renderer/Vulkan/window code

If raw `"walkable"`, `"blocker"`, `"projectile_blocker"`, or `"opening"`
literals remain in `AsciiRoomAssetText.cpp`, classify them in the completion
brief. `shapeName(...)` and `roleName(...)` literals are expected to remain.

## Required Proof

Add or keep focused tests proving:

- role-derived traversal tags still export in the same leading position
- existing valid traversal tags still preserve input order after the required
  role tag
- duplicate role tags are not emitted twice
- unknown/non-catalog traversal tags are still omitted from export

Do not add a broad new test harness if existing ASCII asset text tests can be
extended.

## Acceptance Gates

Run at minimum:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d ascii_room_asset_text_tests ascii_room_asset_text_fixture_tests room_asset_loader_tests traversal_tag_catalog_tests product_ascii_room_activation_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(ascii_room_asset_text_tests|ascii_room_asset_text_fixture_tests|room_asset_loader_tests|traversal_tag_catalog_tests|product_ascii_room_activation_tests)$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

Full CTest is encouraged but not mandatory unless this slice expands beyond the
single export file plus focused tests.

## Required Report

Completion brief must include:

- exact files changed
- which ASCII asset text export insertions now use `TraversalTag`
- whether export order/dedup behavior changed
- remaining raw traversal-like literals in `AsciiRoomAssetText.cpp` and why each
  remains local
- tests/checks run
- receipt golden result
- confirmation that role names, shape names, parser strings, RoomAsset
  validation, movement parsing, collision role strings, ProductAppWindowState,
  and renderer/Vulkan were not changed
