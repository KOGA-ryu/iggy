# E181 — TraversalTag Structural Emitter Migration

## Status

Ready. This is the narrow follow-up to E126. It should reduce traversal-tag
string literal drift without turning into a repo-wide vocabulary sweep.

Commit prefix after review: `claude: planned. codex: ...`.

## Goal

Route the existing structural traversal-tag emitters through the shared
`src/content/assets/TraversalTag.hpp` catalog added in E126, while preserving
the exact emitted strings and output behavior.

This is a behavior-preserving cleanup. The generated RoomAsset and authored-room
payloads should still contain the same traversal tag strings in the same order.

## Scope

Expected implementation touch points:

- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.cpp`
- `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp`
- `src/content/authoring/EditableRoomDocument.cpp`
- focused tests only if they need small expected-output updates or new guards

Use `traversalTagId(TraversalTag::...)` for these cataloged strings where they
are emitted or checked as traversal tags:

- `walkable`
- `blocker`
- `projectile_blocker`
- `opening`, only if this card touches an existing structural emitter/check
- `clamber`
- `clamber_candidate`

It is acceptable to add small local helpers such as `tagString(TraversalTag)` if
needed to convert `std::string_view` to `std::string` cleanly.

## Explicit Non-Scope

Do not migrate:

- movement slot parse tables or movement mechanic display names
  (`vault`, `clamber`, `wire_walk`) in `src/runtime/movement/*`
- collision role display/stringifier helpers in `src/runtime/collision/*`
- `ReasoningGraph` edge labels
- RoomAsset parser role-name stringifiers
- receipt/render/debug/display-only string literals
- ASCII gameplay/material/terrain tags such as `object`, `prop`, `ledge`,
  `crate`, `floor`, `blocked_slope`, `ramp`, `elevated_floor`, or `terrain_*`
- save/load format
- `ProductAppWindowState`
- renderer/Vulkan/window code

If a literal is ambiguous between traversal vocabulary and gameplay/material
vocabulary, leave it local and call it out in the completion brief.

## Required Proof

1. Add or keep focused tests proving output parity for:
   - Creative RoomBake structural surfaces still emit `walkable`, `blocker`,
     and `projectile_blocker` exactly where they did before.
   - ASCII RoomAsset conversion still emits the same structural traversal tags
     and preserves existing `clamber` behavior.
   - ASCII AuthoredRoom conversion still emits `walkable` and
     `clamber_candidate` where existing tests expect them.
2. Run a targeted grep over the touched emitter files and classify remaining raw
   traversal literals.
   - Remaining literals are acceptable when they are stable mesh/surface id
     suffixes, role names, parser/display strings, or non-catalog gameplay tags.
   - The completion brief must list the remaining intentional raw literals.

## Acceptance Gates

Run at minimum:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d traversal_tag_catalog_tests creative_document_room_bake_tests ascii_room_to_room_asset_tests ascii_room_to_authored_room_tests editable_room_document_tests product_ascii_room_activation_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(traversal_tag_catalog_tests|creative_document_room_bake_tests|ascii_room_to_room_asset_tests|ascii_room_to_authored_room_tests|editable_room_document_tests|product_ascii_room_activation_tests)$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

Full CTest is encouraged but not mandatory unless the implementation touches a
broader shared header or changes more than the scoped files above.

## Required Report

Completion brief must include:

- exact files changed
- which emitters/checks were routed through `TraversalTag`
- remaining raw traversal literals in touched files and why each remains local
- tests/checks run
- receipt golden result
- confirmation that movement parsing, collision role strings, save/load,
  ProductAppWindowState, and renderer/Vulkan were not changed

## Completion Brief - E181

- Exact files changed:
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
  - `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.cpp`
  - `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp`
  - `src/content/authoring/EditableRoomDocument.cpp`
  - task card moved from `ready/` to `done/`

- Emitters/checks routed through `TraversalTag`:
  - Creative RoomBake:
    - walkable surface traversal tag now uses
      `traversalTagId(TraversalTag::Walkable)`.
    - actor blocker surface traversal tag now uses
      `traversalTagId(TraversalTag::Blocker)`.
    - projectile blocker surface traversal tag now uses
      `traversalTagId(TraversalTag::ProjectileBlocker)`.
  - ASCII AuthoredRoom conversion:
    - floor semantics traversal tag now uses
      `TraversalTag::Walkable`.
    - wall semantics traversal tag now uses
      `TraversalTag::ClamberCandidate`.
    - movement clamber ledge traversal tag now uses
      `TraversalTag::Clamber`; gameplay tags remain local gameplay vocabulary.
  - ASCII RoomAsset conversion:
    - actor blocker base tag now uses `TraversalTag::Blocker`.
    - projectile blocker tags now use
      `TraversalTag::ProjectileBlocker`.
    - object actor blocker tag now uses `TraversalTag::Blocker`.
    - object walkable top tags now use `TraversalTag::Walkable` and
      `TraversalTag::Clamber`.
    - door blocker tag now uses `TraversalTag::Blocker`.
    - clamber traversal checks now compare against
      `traversalTagId(TraversalTag::Clamber)`.
  - EditableRoom runtime conversion:
    - structural tag insertion/filtering now uses
      `TraversalTag::Walkable`, `TraversalTag::Blocker`, and
      `TraversalTag::ProjectileBlocker`.
    - projectile blocker surface traversal tag now uses
      `TraversalTag::ProjectileBlocker`.
    - clamber wall checks now compare against
      `traversalTagId(TraversalTag::Clamber)`.

- Remaining raw traversal literals in touched files:
  - `RoomBake.cpp`: `"walkable"` remains in `stableObjectId(..., "walkable")`;
    this is a stable runtime surface id suffix, not a traversal tag payload.
  - `RoomBake.cpp`: `"projectile_blocker"` remains in
    `stableObjectId(..., "projectile_blocker")`; this is a stable runtime
    surface id suffix, not a traversal tag payload.
  - `AsciiRoomToAuthoredRoom.cpp`: `"clamber"` remains in
    `semantics.gameplayTags`; this is the existing gameplay/material tag copy
    for the clamber ledge object and is explicitly outside the traversal
    payload migration.

- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d traversal_tag_catalog_tests creative_document_room_bake_tests ascii_room_to_room_asset_tests ascii_room_to_authored_room_tests editable_room_document_tests product_ascii_room_activation_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(traversal_tag_catalog_tests|creative_document_room_bake_tests|ascii_room_to_room_asset_tests|ascii_room_to_authored_room_tests|editable_room_document_tests|product_ascii_room_activation_tests)$' --output-on-failure` passed: 6/6.
  - Full CTest passed: `100% tests passed, 0 tests failed out of 260`
    (`/tmp/iggy3d_e181_ctest.log`).
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`
    produced no output.
  - Focused trailing-whitespace scan over touched files produced no output.

- Receipt golden result:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` passed:
    `receipt key-order oracle: 1032 fields match golden (order + values)`.

- Scope confirmations:
  - Movement parsing and movement mechanic display names were not changed.
  - Collision role strings/stringifiers were not changed.
  - Save/load format was not changed.
  - `ProductAppWindowState` was not changed.
  - Renderer/Vulkan/window code was not changed.

- Concerns/deferred:
  - None for E181. Non-catalog ASCII gameplay/material/terrain tags such as
    `object`, `prop`, `ledge`, `blocked_slope`, `ramp`, and `terrain_*` remain
    local by card scope.
