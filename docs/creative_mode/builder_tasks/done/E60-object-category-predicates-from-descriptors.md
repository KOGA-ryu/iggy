# E60: Object Category Predicates From Descriptors

## Objective

Remove or migrate the legacy `isStructuralObject(...)`-style switch ladders so
object category checks read from `ObjectDescriptor` instead of duplicating the
object-kind taxonomy.

## Problem

`ObjectDescriptor` already stores `CreativeObjectCategory`, and
`ObjectDescriptor.cpp` exposes `categoryOf(...)` / `objectUsesCategory(...)`.
But `Object.cpp` still maintains old category predicates as separate switches:

- `isStructuralObject(...)`
- `isTerrainOrVolumeObject(...)`
- `isNavigationOrMovementObject(...)`
- `isLogicObject(...)`
- `isTestingObject(...)`
- `isVisualDressingObject(...)`
- `isLightSoundOrCameraObject(...)`
- `isAuthoringMetaObject(...)`
- `isGameplayObject(...)`

Evidence:

- Declarations live in `src/app/iggy3d/creative/document/Object.hpp:197`.
- Implementations live in `src/app/iggy3d/creative/document/Object.cpp:156`.
- `rg` shows the only current non-implementation use is
  `tests/unit/creative_room_tests.cpp:153`.

These switches are exactly the old per-kind table debt the descriptor registry
was meant to replace. If a future object kind is added, category can now drift
between the descriptor row and legacy predicates.

## Required Reads

- `src/app/iggy3d/creative/document/Object.hpp`
- `src/app/iggy3d/creative/document/Object.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `tests/unit/creative_room_tests.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`

## Scope

- Prefer deleting the legacy predicates if production callers no longer need
  them.
- If public compatibility is still useful, reimplement them as thin wrappers
  around `objectUsesCategory(...)` / `categoryOf(...)` instead of switches.
- Move category predicate tests to descriptor tests or update them to prove the
  wrappers delegate to descriptor category truth.
- Preserve public behavior for existing callers.

## Acceptance

- No hand-maintained per-kind category switch remains outside the descriptor
  registry.
- `CreativeObjectKind` category truth has one owner: the descriptor row.
- Existing tests for Room classification still pass, but they no longer bless a
  duplicate category table.
- Adding a new object kind only requires updating the descriptor row for category
  classification.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_room_tests creative_object_descriptor_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_room_tests|creative_object_descriptor_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change object categories.
- Do not remove `toString(CreativeObjectKind)` in this card.
- Do not change save/load enum values.
- Do not combine this with mutation metadata cleanup from E49.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/document/Object.cpp`
  - `tests/unit/creative_room_tests.cpp`
  - `tests/unit/creative_object_descriptor_tests.cpp`
- Implementation:
  - Replaced all legacy object category predicate switch ladders with thin
    wrappers around `objectUsesCategory(...)`.
  - Kept the public compatibility predicate names intact for existing callers.
  - Updated the Room classification test to assert descriptor category truth.
  - Added descriptor test coverage proving every legacy predicate follows the
    descriptor category for every known `CreativeObjectKind`.
- Verification:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing whitespace scan over touched files passed.
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_room_tests creative_object_descriptor_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_room_tests|creative_object_descriptor_tests)$' --output-on-failure` passed.
- Concerns:
  - None for this slice. Category truth now has one owner: the descriptor row.
