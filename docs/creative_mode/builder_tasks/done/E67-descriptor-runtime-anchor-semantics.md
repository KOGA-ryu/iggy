# E67: Descriptor Runtime Anchor Semantics

## Objective

Give Creative point descriptors an explicit runtime anchor semantic so RoomBake
can emit product-meaningful anchor kinds without guessing from broad occupancy.

## Problem

RoomBake currently admits point anchors through descriptor facts, but derives
the emitted `RoomAnchorAsset.kind` from `CreativeSpatialOccupancyKind`:

- Navigation points become `navigation`.
- Gameplay points become `gameplay`.
- Light/audio/camera points become broad metadata roles.

That is not enough for product runtime session seeding. The product package seed
path expects concrete anchor vocabulary such as `spawn`, `exit`, `npc`,
`pickup`, `key`, `treasure`, `door`, and `secret_door`; otherwise the anchor is
only a generic marker, and a room with Creative `SpawnPoint` can still fail as
missing a `spawn` anchor.

This is a code-quality issue, not only a missing feature: tests can stay green
by asserting "one anchor was baked" while the baked anchor is semantically
unusable downstream.

## Evidence

- `src/app/iggy3d/creative/adapters/RoomBake.cpp:99` admits anchors from broad
  occupancy classes.
- `src/app/iggy3d/creative/adapters/RoomBake.cpp:243` maps occupancy to
  generic strings like `navigation` and `gameplay`.
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp:143` defines
  `SpawnPoint` as a Navigation point, so it currently bakes as `navigation`,
  not `spawn`.
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp:144` defines
  `ExitPoint` as the same broad Navigation shape, so it cannot become `exit`
  without another semantic.
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp:217` and nearby
  gameplay markers share broad Gameplay occupancy, so `NpcSpawn`, `LootPoint`,
  and related markers collapse to `gameplay`.
- `src/app/iggy3d/world/PackageSessionSeed.cpp:152` treats only `spawn` as
  the spawn anchor.
- `src/app/iggy3d/world/PackageSessionSeed.cpp:162` maps `npc`/`monster`,
  `pickup`/`key`/`treasure`, `door`/`secret_door`, and `exit` specially.
- `src/app/iggy3d/world/PackageSessionSeed.cpp:327` fails room seeding when no
  `spawn` anchor is present.

## Required Reads

- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `src/app/iggy3d/creative/adapters/RoomBake.hpp`
- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- `src/app/iggy3d/world/PackageSessionSeed.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`
- `tests/unit/creative_document_room_bake_tests.cpp`

## Scope

- Add a descriptor-owned runtime anchor semantic, preferably a small enum or
  stable string field on `CreativeObjectDescriptor`.
- Keep the semantic empty/unknown for descriptors that should only bake as
  broad metadata anchors.
- Make RoomBake use the descriptor semantic for product-meaningful anchors.
- Preserve broad non-session metadata anchors where they are still useful, such
  as light/audio/camera, without pretending they are gameplay spawn points.
- Add focused tests that prove concrete point descriptors bake to concrete
  anchor kinds.

## Acceptance

- `SpawnPoint` bakes to anchor kind `spawn`.
- `ExitPoint` bakes to anchor kind `exit`.
- At least one actor/pickup descriptor gets a product-meaningful kind, for
  example `NpcSpawn -> npc` and/or `LootPoint -> pickup` if those semantics are
  accepted.
- Descriptors with no product runtime anchor semantic still do not accidentally
  become `spawn`/`exit`/`npc` just because they share Navigation or Gameplay
  occupancy.
- RoomBake anchor tests assert downstream-useful `anchor.kind` values, not only
  `bakedAnchorCount`.
- No object-kind switch is added inside RoomBake for anchor kind derivation; the
  descriptor owns the semantic.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_room_bake_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_room_bake_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not add a per-kind anchor table in `RoomBake.cpp`.
- Do not implement the full anchor-to-session-seed bridge unless the descriptor
  semantic and RoomBake output are already clean and the diff stays focused.
- Do not change static mesh or spatial surface bake policy.
- Do not use broad `Navigation` or `Gameplay` occupancy as a synonym for
  product session semantics.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
  - `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
  - `tests/unit/creative_object_descriptor_tests.cpp`
  - `tests/unit/creative_document_room_bake_tests.cpp`
- Added descriptor-owned `CreativeRuntimeAnchorSemantic` with stable string
  output:
  - `None -> ""`
  - `Spawn -> "spawn"`
  - `Exit -> "exit"`
  - `Npc -> "npc"`
  - `Monster -> "monster"`
  - `Pickup -> "pickup"`
  - `Light -> "light"`
  - `Audio -> "audio"`
  - `Camera -> "camera"`
- Added `CreativeObjectDescriptor::runtimeAnchorSemantic`, defaulting to
  `None`.
- Assigned explicit runtime anchor semantics on descriptor rows:
  - `SpawnPoint -> Spawn`
  - `ExitPoint -> Exit`
  - `EnemySpawn -> Monster`
  - `NpcSpawn -> Npc`
  - `LootPoint -> Pickup`
  - `PointLight` / `SpotLight -> Light`
  - `SoundEmitter -> Audio`
  - `CameraMarker` / `CameraTarget -> Camera`
- Changed RoomBake point-anchor eligibility and `RoomAnchorAsset.kind`
  derivation to use `descriptor.runtimeAnchorSemantic`; broad
  `Navigation`/`Gameplay`/`Light`/`Audio`/`Camera` occupancy no longer maps to
  anchor kinds by itself.
- Added descriptor tests for semantic strings and representative descriptor
  rows, including rows that intentionally remain `None`.
- Added RoomBake tests proving:
  - `SpawnPoint` bakes as `spawn`
  - `ExitPoint` bakes as `exit`
  - `NpcSpawn` bakes as `npc`
  - `LootPoint` bakes as `pickup`
  - `EnemySpawn` bakes as `monster`
  - broad `Navigation`/`Gameplay` points without descriptor semantics do not
    bake anchors.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_room_bake_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_room_bake_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.
