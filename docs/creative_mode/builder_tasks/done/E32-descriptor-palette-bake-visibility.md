# E32: Descriptor Palette and Bake Visibility Policy

## Objective

Make standalone brush palette visibility agree with runtime/bake visibility for
non-geometry descriptors, especially BoxVolume/test descriptors.

## Problem

The standalone brush predicate currently admits BoxVolume descriptors when they
have transform, bounds, and BoxProjection. RoomBake now deliberately excludes all
BoxVolume descriptors from static geometry. Current descriptor rows affected by
this shape/projection mismatch include testing volumes such as `TestLane`,
`FallShaft`, and `TimingGate`.

This may be a valid authoring/debug use case, but it should be explicit. A user
should not be able to place an object from the brush palette and have it vanish
from baked preview unless the descriptor/palette says it is non-runtime helper
geometry.

## Required Reads

- `apps/iggy3d_creative/main.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`
- `tests/unit/creative_document_room_bake_tests.cpp`

## Scope

- Audit every currently placeable descriptor against standalone preview,
  RoomBake output, and authoring/runtime intent.
- Add a descriptor-driven palette/bake visibility policy if the current fields
  are not expressive enough.
- Keep existing editor-only filtering, but do not rely on a kind-name deny list.

## Acceptance

- Brush-placeable descriptors have a clear expected preview/bake outcome.
- BoxVolume descriptors are either intentionally hidden from the standalone brush
  palette or have a tested non-static preview/metadata path.
- Tests pin the intended behavior for `TestLane`, `FallShaft`, `TimingGate`, and
  at least one VolumeProjection gameplay volume.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d iggy3d_creative creative_object_descriptor_tests creative_document_room_bake_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_room_bake_tests)$' --output-on-failure`
- Run a standalone capture if standalone palette behavior changes.
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not silently re-enable BoxVolume static geometry in RoomBake.
- Do not add per-kind palette deny rows unless a descriptor field cannot solve
  the policy.
- Do not change Room metadata semantics here.

## Completion Brief

- Files changed:
  - `apps/iggy3d_creative/main.cpp`
  - `tests/unit/creative_object_descriptor_tests.cpp`
  - `tests/unit/creative_document_room_bake_tests.cpp`
- Behavior changed:
  - Standalone brush palette hygiene now excludes descriptors with
    `shapeKind == BoxVolume` unless/until they get a tested non-static
    preview/metadata path.
  - Existing editor-only filtering remains.
  - RoomBake policy remains conservative: BoxVolume descriptors do not emit
    static meshes/surfaces.
  - No Room metadata semantics changed.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d iggy3d_creative creative_object_descriptor_tests creative_document_room_bake_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_room_bake_tests)$' --output-on-failure`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e32_final.png --frames 32 > /tmp/iggy3d_creative_e32_final.log 2>&1`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[[:blank:]]$" /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp /Users/kogaryu/iggy3d/tests/unit/creative_object_descriptor_tests.cpp /Users/kogaryu/iggy3d/tests/unit/creative_document_room_bake_tests.cpp /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E32-descriptor-palette-bake-visibility.md`
- Evidence:
  - `creative_object_descriptor_tests` and
    `creative_document_room_bake_tests` passed.
  - Capture hash stayed
    `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`.
  - Capture log:
    `brush palette hygiene before=79 after=66 removed=13 predicate='!descriptor.isEditorOnly && shapeKind!=BoxVolume' removed='TestLane,FallShaft,TimingGate,...'`
  - Capture log kept `ROUNDTRIP objectCount before=8 afterClear=0 afterLoad=8 match=1`.
  - Capture log kept `ROOM_BAKE final ... staticMeshes=6 spatialSurfaces=11 ... standalonePreviewMeshes=3 sceneMeshes=1690`.
- Concerns/deferred:
  - BoxVolume authoring can return to the brush palette later, but it needs an
    explicit non-static preview/metadata path instead of pretending it bakes as
    runtime static geometry.
