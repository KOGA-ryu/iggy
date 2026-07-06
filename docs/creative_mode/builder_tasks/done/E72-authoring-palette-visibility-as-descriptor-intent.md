# E72: Authoring Palette Visibility As Descriptor Intent

## Objective

Make authoring palette visibility an explicit descriptor intent, not a broad
shape-derived rule that happens to match today's examples.

## Problem

E41 moved standalone brush palette visibility out of `main.cpp`, but the new
shared helper still derives visibility from a broad rule:

```cpp
descriptor.kind != Unknown &&
!descriptor.isEditorOnly &&
descriptor.shapeKind != BoxVolume
```

This centralizes the policy, but it does not make visibility a per-descriptor
fact. It means every future `BoxVolume` descriptor is hidden from the authoring
brush palette by default, even if a specific future volume should be placeable
through the palette. Tests now pin representative current rows, but they also
green-light this coarse rule as if it were descriptor truth.

## Required Reads

- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `apps/iggy3d_creative/main.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`
- `docs/creative_mode/builder_tasks/done/E41-descriptor-owned-palette-visibility.md`

## Scope

- Add an explicit descriptor/catalog field or enum for authoring palette
  visibility, for example `AuthoringPaletteVisibility::{Hidden, Brush}`.
- Initialize current rows to preserve E41 behavior:
  - editor-only rows hidden;
  - current BoxVolume rows hidden;
  - current non-editor Point/Line/Path/Surface/MeshProxy rows visible when
    placement support allows them.
- Reimplement `descriptorShowsInAuthoringBrushPalette(...)` from that explicit
  field/enum, not directly from shape kind.
- Keep standalone behavior and capture output stable.

## Acceptance

- A future descriptor can opt a BoxVolume-like object into or out of the
  authoring palette without changing global shape policy.
- Tests prove at least one hidden non-editor BoxVolume and one visible
  non-BoxVolume row, plus editor-only hidden rows.
- The helper remains shared outside standalone.
- Standalone palette logs may change predicate text, but object visibility and
  capture remain stable.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d iggy3d_creative creative_object_descriptor_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_object_descriptor_tests$' --output-on-failure`
- `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e72_final.png --frames 32 > /tmp/iggy3d_creative_e72_final.log 2>&1`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not expose all descriptors in the palette.
- Do not change placement support rules in this card.
- Do not reclassify RoomBake static geometry.
- Do not use a kind-name deny list in standalone.

## Completion Brief

- Status: done.
- Files modified:
  - `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
  - `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
  - `tests/unit/creative_object_descriptor_tests.cpp`
- Implementation:
  - Added `CreativeAuthoringPaletteVisibility` with values `Hidden` and `Brush`.
  - Added `CreativeObjectDescriptor::authoringPaletteVisibility`, defaulting to
    `Hidden`.
  - Added row-level `kAuthoringBrushPalette` descriptor capability and mapped it
    to `CreativeAuthoringPaletteVisibility::Brush` during descriptor
    construction.
  - Tagged the current rows that matched the previous shared helper policy:
    non-Unknown, non-editor-only, non-BoxVolume descriptors.
  - Reimplemented `descriptorShowsInAuthoringBrushPalette(...)` to read the
    explicit descriptor field instead of recomputing from shape/editor flags.
  - Standalone still applies its separate placement-support filter, so object
    visibility and capture output stayed stable.
- Tests:
  - Descriptor tests now assert the helper follows
    `authoringPaletteVisibility` directly.
  - Pinned Room and TestLane as explicit hidden descriptors, Crate as explicit
    visible, and editor-only Note as explicit hidden.
- Capture evidence:
  - `/tmp/iggy3d_creative_e72_final.png`
  - SHA-256: `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`
  - Palette log stayed `before=79 after=66 removed=13` with predicate
    `descriptorShowsInAuthoringBrushPalette`.
  - Final submit reason stayed `package_room_meshes_presented`.
  - Visual inspection showed the expected stable standalone scene: floor, wall,
    crates, beam, point marker, path route, grid, UI, and green ghost.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d iggy3d_creative creative_object_descriptor_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_object_descriptor_tests$' --output-on-failure`
  - `./build/iggy3d_creative --capture /tmp/iggy3d_creative_e72_final.png --frames 32 > /tmp/iggy3d_creative_e72_final.log 2>&1`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
- Result: all passed.
