# E180 — Affordance Anchor Wire Strings

## Status

Ready. Implement only after the E179 runtime-state-hash deletion is committed and
the worktree is otherwise clean.

Commit prefix after review: `claude: planned. codex: ...`.

## Goal

Close the existing authoring-to-AI handshake for the already-live reasoning
anchor vocabulary:

- Creative `CoverPoint` should bake a RoomAsset anchor with kind `cover`.
- Creative `PatrolNode` should bake a RoomAsset anchor with kind `patrol_post`.
- ASCII `monster_spawn` should bake a RoomAsset anchor with kind `monster`, not
  `npc`.

The runtime reader already understands these strings in
`src/runtime/ai/ReasoningGraph.cpp::nodeKindForAnchor(...)`. This card wires the
emitters to the existing reader contract and adds a proving test so silent
string drift cannot regress.

## Scope

Expected source touch points:

- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.cpp`
- focused unit tests only

Do not touch:

- save/load format
- movement/runtime AI scoring policy
- reasoning graph node semantics
- renderer/Vulkan/window code
- ProductAppWindowState/store decomposition
- UI/menu/input routing

## Required Implementation

1. Creative descriptor anchor semantics
   - Append `Cover` and `PatrolPost` to `CreativeRuntimeAnchorSemantic`.
   - Set the `CoverPoint` descriptor to `CreativeRuntimeAnchorSemantic::Cover`.
   - Set the `PatrolNode` descriptor to
     `CreativeRuntimeAnchorSemantic::PatrolPost`.
   - Extend `toString(CreativeRuntimeAnchorSemantic)` with byte-exact strings:
     - `Cover` -> `cover`
     - `PatrolPost` -> `patrol_post`

2. ASCII monster spawn anchor
   - In `AsciiRoomToRoomAsset.cpp`, split the current collapsed branch:
     `npc_spawn` remains `npc`; `monster_spawn` returns `monster`.
   - Keep downstream session/reachability behavior unchanged. Existing
     downstream code already treats `monster` as a usable runtime anchor where
     appropriate.

3. Proving tests
   - Add or extend a focused test that authors/bakes a creative room with one
     `CoverPoint` and one `PatrolNode`, then builds a reasoning graph and proves
     it contains:
     - one `ReasoningNodeKind::coverCluster` from the `cover` anchor
     - one `ReasoningNodeKind::patrolPost` from the `patrol_post` anchor
   - Add or extend an ASCII conversion test proving `monster_spawn` now emits an
     anchor kind of `monster`, while `npc_spawn` still emits `npc`.
   - Keep tests deterministic and no-window.

## Acceptance Gates

Run at minimum:

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_room_bake_tests|reasoning_graph_tests|ascii_room_to_room_asset_tests|ascii_room_package_fixture_tests|product_ascii_room_activation_tests)$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

Full CTest is encouraged because this changes shared content vocabulary. Do not
launch a window.

## Required Report

Completion brief must include:

- exact files changed
- exact enum/string mappings added
- exact descriptor rows changed
- whether `monster_spawn` changed from `npc` to `monster`
- test commands and results
- receipt golden result
- confirmation that save/load, ProductAppWindowState, renderer, and runtime AI
  scoring were not changed

## Follow-Up Not In This Card

`chokepoint`, `high_ground`, and `hiding_spot` are also accepted by the
reasoning reader, but no current ASCII glyphs or Creative object kinds author
them. That is new authoring surface and belongs in a separate card.

## Completion Brief - E180

- Exact files changed:
  - `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
  - `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
  - `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.cpp`
  - `tests/unit/creative_object_descriptor_tests.cpp`
  - `tests/unit/creative_document_room_bake_tests.cpp`
  - `tests/unit/ascii_room_to_room_asset_tests.cpp`
  - task card moved from `ready/` to `done/`

- Exact enum/string mappings added:
  - `CreativeRuntimeAnchorSemantic::Cover -> "cover"`
  - `CreativeRuntimeAnchorSemantic::PatrolPost -> "patrol_post"`

- Exact descriptor rows changed:
  - `CreativeObjectKind::CoverPoint` now uses
    `CreativeRuntimeAnchorSemantic::Cover`.
  - `CreativeObjectKind::PatrolNode` now uses
    `CreativeRuntimeAnchorSemantic::PatrolPost`.

- ASCII monster spawn mapping:
  - `npc_spawn` remains `npc`.
  - `monster_spawn` changed from the collapsed `npc` branch to `monster`.

- Proving tests added/updated:
  - `creative_object_descriptor_tests` now pins the new semantic strings and
    the `CoverPoint` / `PatrolNode` descriptor rows.
  - `creative_document_room_bake_tests` now creates a `CoverPoint` and a
    `PatrolNode`, bakes them to RoomAsset anchors, builds a reasoning graph,
    and proves one `ReasoningNodeKind::coverCluster` comes from `cover` and
    one `ReasoningNodeKind::patrolPost` comes from `patrol_post`.
  - `ascii_room_to_room_asset_tests` now builds an inline room with `N` and
    `M`, proving `npc_spawn -> npc` and `monster_spawn -> monster`.

- Test commands and results:
  - `cmake --build /Users/kogaryu/iggy3d/build --target creative_object_descriptor_tests creative_document_room_bake_tests reasoning_graph_tests ascii_room_to_room_asset_tests ascii_room_package_fixture_tests product_ascii_room_activation_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_room_bake_tests|reasoning_graph_tests|ascii_room_to_room_asset_tests|ascii_room_package_fixture_tests|product_ascii_room_activation_tests)$' --output-on-failure` passed: 6/6.
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` passed:
    `receipt key-order oracle: 1032 fields match golden (order + values)`.
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed. Existing unrelated
    warnings remain in starter/menu test initializers and one unused test
    variable.
  - Full CTest passed: `100% tests passed, 0 tests failed out of 260`
    (`/tmp/iggy3d_e180_ctest.log`).
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`
    produced no output.
  - Focused trailing-whitespace scan over touched files produced no output.

- Scope confirmations:
  - Save/load format was not changed.
  - `ProductAppWindowState` and store decomposition were not changed.
  - Renderer/Vulkan/window code was not changed.
  - Runtime AI scoring and `ReasoningGraph.cpp` node semantics were not
    changed; this slice only wired emitters to existing accepted strings.

- Concerns/deferred:
  - None for E180. `chokepoint`, `high_ground`, and `hiding_spot` remain future
    authoring-surface work as stated in the card.
