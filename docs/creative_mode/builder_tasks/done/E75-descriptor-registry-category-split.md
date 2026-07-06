# E75: Descriptor Registry Category Split

## Objective

Split the single `CreativeObjectDescriptor` registry wall into smaller
category/profile-owned sections without changing descriptor facts.

## Problem

E71 made each descriptor row readable, but `ObjectDescriptor.cpp` is now almost
2,000 lines and still owns every descriptor row in one `kDescriptors` array.
Adding a new object kind still requires editing a giant mixed-purpose registry
that interleaves structural geometry, volumes, navigation, logic, testing,
dressing, sensory, authoring, and gameplay rows.

This is better than dense one-line rows, but it is still a feature-add bottleneck:
reviewing one new gameplay marker requires scrolling through unrelated room,
terrain, test, and authoring metadata rows.

## Required Reads

- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `tests/unit/creative_object_descriptor_tests.cpp`
- `docs/creative_mode/builder_tasks/done/E71-descriptor-table-row-layout.md`
- `docs/creative_mode/builder_tasks/done/E72-authoring-palette-visibility-as-descriptor-intent.md`

## Evidence

- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp` is currently 1,983
  lines.
- The registry starts as one `constexpr auto kDescriptors =
  std::to_array<CreativeObjectDescriptor>({...})` at about line 125.
- Rows are already naturally grouped by `CreativeObjectCategory`, but those
  groups are not named or isolated; the public accessor still exposes only the
  final flattened array.

## Scope

- Preserve every descriptor row, row order, serialized name, display name,
  purpose string, dirty flag, default bounds, runtime anchor semantic,
  authoring palette visibility, and capability fact.
- Introduce small category-local descriptor arrays or named section builders,
  then stitch them into the same public `allObjectDescriptors()` order.
- Keep `describeObject(...)`, `allObjectDescriptors()`, and descriptor helper
  API behavior unchanged.
- Keep compile-time construction if practical.

## Acceptance

- The descriptor registry is split into named sections that match the current
  object taxonomy, so a reviewer can inspect one category without scanning the
  entire table.
- `allObjectDescriptors()` returns the same descriptor count and same order as
  before the split.
- Focused descriptor tests still pin representative taxonomy, projection,
  palette visibility, runtime anchor semantics, and dirty flags.
- No descriptor semantics change.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_create_tests creative_document_room_bake_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_create_tests|creative_document_room_bake_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

## Do Not

- Do not add, remove, or reclassify descriptor rows.
- Do not change object kind serialization, palette visibility, RoomBake
  behavior, mutation policy, or runtime anchor semantics.
- Do not move descriptor facts into app-local code.

## Completion Notes

- Split `ObjectDescriptor.cpp` into ten named descriptor sections:
  `kUnknownDescriptors`, `kStructuralDescriptors`,
  `kTerrainOrVolumeDescriptors`, `kNavigationOrMovementDescriptors`,
  `kLogicDescriptors`, `kTestingDescriptors`, `kVisualDressingDescriptors`,
  `kLightSoundOrCameraDescriptors`, `kAuthoringMetaDescriptors`, and
  `kGameplayDescriptors`.
- Flattened those sections back into `kDescriptors` with a constexpr
  `concatDescriptorSections(...)` helper so `allObjectDescriptors()` keeps the
  same public order and API shape.
- Repaired the initial mechanical split before testing because it dropped
  several structural rows; final registry contains 108 descriptor rows.
- Verified:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_create_tests creative_document_room_bake_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_create_tests|creative_document_room_bake_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over `ObjectDescriptor.cpp`
