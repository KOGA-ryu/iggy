# E126: TraversalTag Catalog Contract And Validator Parity

## Objective

Introduce the first authoritative `TraversalTag` catalog and route only the
closed traversal-tag validators through it.

This is the implementation follow-up to E122. Keep it deliberately narrow:
create the catalog, pin the string/parse contract, and make the three existing
validators agree. Do not migrate all emitters/consumers in this slice.

## Owner Decision

Retain `clamber_candidate` as a valid traversal tag in the catalog for now, but
classify it as an authoring/runtime-content hint, not as a movement traversal
slot.

Evidence from E122:

- `AsciiRoomToAuthoredRoom.cpp` emits `clamber_candidate` for authored wall
  semantics.
- `EditableRoomDocument.cpp::validSemanticTag(...)` accepts it.
- `RoomAsset.cpp::validTraversalTag(...)` and
  `AsciiRoomAssetText.cpp::parserCompatibleTraversalTag(...)` reject/drop it.
- `MovementTraversalSlots.cpp` only maps `vault`, `clamber`, and `wire_walk`
  to movement slots; it does not consume `clamber_candidate`.

So the safest first fix is validator parity: existing authored data should not
pass one validator and fail/drop at the RoomAsset boundary. Movement slot policy
stays local and unchanged.

## Required Work

1. Add a content-asset catalog.
   - Suggested files:
     - `src/content/assets/TraversalTag.hpp`
     - `src/content/assets/TraversalTag.cpp`
   - Register the `.cpp` in `CMakeLists.txt` if the project uses explicit source
     lists.
2. Add a closed enum and string table.
   - Required values:
     - `Walkable` -> `walkable`
     - `Blocker` -> `blocker`
     - `ProjectileBlocker` -> `projectile_blocker`
     - `Opening` -> `opening`
     - `Clamber` -> `clamber`
     - `ClamberCandidate` -> `clamber_candidate`
     - `Vault` -> `vault`
     - `WireWalk` -> `wire_walk`
     - `NoPlayer` -> `no_player`
     - `DebugOnly` -> `debug_only`
   - Suggested API:
     - `enum class TraversalTag`
     - `std::string_view traversalTagId(TraversalTag) noexcept`
     - `std::optional<TraversalTag> parseTraversalTag(std::string_view) noexcept`
       or a local-style bool/out-param parser
     - `bool validTraversalTag(std::string_view) noexcept`
     - `std::span<const TraversalTag> allTraversalTags() noexcept` if local C++
       standard/style supports it; otherwise return an existing-project-friendly
       view.
     - Optional category helpers:
       - structural role tags: `walkable`, `blocker`, `projectile_blocker`,
         `opening`
       - movement slot tags: `vault`, `clamber`, `wire_walk`
       - authoring hint tags: `clamber_candidate`
3. Add focused catalog tests.
   - Suggested file/target:
     - `tests/unit/traversal_tag_catalog_tests.cpp`
     - `traversal_tag_catalog_tests`
   - Pin:
     - every enum value stringifies to the exact id above;
     - every id parses back to the enum;
     - an unknown string does not parse;
     - `clamber_candidate` is valid/cataloged;
     - category helpers, if added, do **not** classify `clamber_candidate` as a
       movement slot.
4. Route the three validators only.
   - `src/content/authoring/EditableRoomDocument.cpp::validSemanticTag(...)`
   - `src/content/assets/RoomAsset.cpp::validTraversalTag(...)`
   - `src/app/iggy3d/ascii_room/AsciiRoomAssetText.cpp::parserCompatibleTraversalTag(...)`
   All three should answer from the catalog and therefore accept the same closed
   set.
5. Add or update validator parity tests.
   - Prove all three validation paths accept the full catalog set, including
     `clamber_candidate`.
   - Prove at least one unknown tag still rejects.
   - Add a RoomAsset or asset-text validation regression proving a
     `clamber_candidate` traversal tag no longer fails/drops at the RoomAsset
     boundary.

## Acceptance Notes

- `clamber_candidate` is retained as valid content vocabulary.
- Movement slot parsing remains unchanged: only `vault`, `clamber`, and
  `wire_walk` become movement traversal slots.
- No broad literal migration is done in this card. E127 will replace hand-written
  literals in emitters/consumers after this contract is stable.
- ASCII authored-room strings such as `object`, `prop`, `ledge`, `crate`,
  `floor`, `blocked_slope`, `ramp`, `elevated_floor`, and `terrain_*` are
  explicitly out of scope. Do not bless them as traversal tags in this card.

## Do Not

- Do not change RoomBake emission policy.
- Do not change movement traversal slot behavior.
- Do not move ASCII gameplay/material/terrain strings between tag vectors.
- Do not migrate every traversal string literal in the repo.
- Do not touch renderer, save/load kernel, product input, Vulkan, or receipts
  except tests that already exercise validation.
- Do not stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d traversal_tag_catalog_tests creative_document_room_bake_tests product_ascii_authoring_smoke product_ascii_room_activation_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(traversal_tag_catalog_tests|creative_document_room_bake_tests|product_ascii_authoring_smoke|product_ascii_room_activation_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Catalog API:
- Validators routed:
- `clamber_candidate` regression proof:
- Tests/checks run:
- Concerns/deferred:
