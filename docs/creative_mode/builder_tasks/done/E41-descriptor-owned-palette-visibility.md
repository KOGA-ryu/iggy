# E41: Descriptor-Owned Palette Visibility

## Objective

Move standalone brush/palette visibility policy out of app-local shape filters
and into a descriptor/catalog fact.

## Problem

E32 correctly stopped BoxVolume descriptors from appearing in the standalone
brush palette, but the policy now lives in `apps/iggy3d_creative/main.cpp`:

```cpp
!descriptor.isEditorOnly && descriptor.shapeKind != BoxVolume
```

That is descriptor-driven in a narrow sense, but it is still local policy in the
standalone app. Product Creative UI and future descriptor-backed palettes can
drift because there is no explicit descriptor/catalog field for “show this in an
authoring brush palette.”

## Required Reads

- `apps/iggy3d_creative/main.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp/.cpp`
- `src/app/iggy3d/creative/tools/Palette.hpp/.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`
- `tests/unit/creative_document_room_bake_tests.cpp`

## Scope

- Add a descriptor/catalog-owned palette visibility fact or helper, e.g.
  `showInStandaloneBrushPalette`, `editorPaletteVisibility`, or a similarly
  explicit predicate owned outside the standalone app.
- Keep current E32 behavior: editor-only descriptors and BoxVolume descriptors
  stay out of the standalone brush palette.
- Keep Room metadata and RoomBake semantics unchanged.

## Acceptance

- Standalone no longer embeds the BoxVolume visibility rule directly.
- Descriptor tests pin palette visibility for `Room`, `Crate`, `Wall`, `Beam`,
  `PointLight`, `PatrolRoute`, `TestLane`, `FallShaft`, `TimingGate`, and at
  least one editor-only descriptor.
- Standalone capture remains visually stable unless logging changes only.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d iggy3d_creative creative_object_descriptor_tests creative_document_room_bake_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_room_bake_tests)$' --output-on-failure`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e41_final.png --frames 32 > /tmp/iggy3d_creative_e41_final.log 2>&1`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not re-enable BoxVolume brush placement.
- Do not use a kind-name deny list.
- Do not change RoomBake static geometry policy.

## Completion Brief

Status: done.

Files changed:
- `apps/iggy3d_creative/main.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`

Behavior changed:
- Added descriptor-owned palette visibility helpers:
  - `descriptorShowsInAuthoringBrushPalette(...)`
  - `objectShowsInAuthoringBrushPalette(...)`
- Centralized current palette visibility policy outside standalone:
  valid non-editor descriptors are visible unless their shape is `BoxVolume`.
- Standalone brush palette now calls the descriptor helper instead of embedding
  `!descriptor.isEditorOnly && descriptor.shapeKind != BoxVolume`.
- RoomBake policy and Room metadata behavior are unchanged.

Tests/checks run:
- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d iggy3d_creative creative_object_descriptor_tests creative_document_room_bake_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_room_bake_tests)$' --output-on-failure`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e41_final.png --frames 32 > /tmp/iggy3d_creative_e41_final.log 2>&1`
- `git -C /Users/kogaryu/iggy3d diff --check`
- focused trailing-whitespace scan over touched files

Evidence:
- Build passed.
- Focused CTest passed: 2/2 tests.
- Capture passed with SHA-256
  `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`.
- Capture submit reason stayed `package_room_meshes_presented`.
- Final `ROOM_BAKE` line stayed accepted with `staticMeshes=6`,
  `spatialSurfaces=11`, `skippedUnsupported=1`, `standalonePreviewMeshes=3`,
  `sceneMeshes=1690`.
- Palette log: `before=79 after=66 removed=13` and predicate now reports
  `descriptorShowsInAuthoringBrushPalette`.
- Visual inspection showed the expected standalone scene: Floor/Wall/Crates,
  Beam, Point marker, PatrolRoute path, grid, UI, and green ghost.
- Descriptor tests pin palette visibility for Room, Crate, Wall, Beam,
  PointLight, PatrolRoute, TestLane, FallShaft, TimingGate, Note, and
  MeasurementLine.

Concerns/deferred:
- Placement support shape checks still live in standalone because this task only
  moved palette visibility policy. A later task can move placement-affordance
  support into descriptor/tool truth if needed.
