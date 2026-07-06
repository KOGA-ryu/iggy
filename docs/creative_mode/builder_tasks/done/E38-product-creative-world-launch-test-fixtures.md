# E38: Product Creative World Launch Test Fixtures

## Objective

Reduce the 3,825-line `product_creative_world_launch_tests.cpp` feature-add
tax by extracting shared Creative launch/input/bake fixtures.

## Problem

The test file now covers launch/open/save identity, RoomBake refresh, manual
rebuild, auto-refresh, delete, undo, move, and generated Room shell behavior.
Many tests rebuild similar creative worlds and click UI rows through repeated
helpers. The tests are valuable, but the file is large enough that new features
encourage copy/paste instead of intent-focused assertions.

## Required Reads

- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`
- Existing test helper patterns in nearby product tests

## Scope

- Extract shared no-window creative launch, row-click, seeded-room, seeded-bake,
  and sentinel active-room helpers into a local helper section or a small test
  support header if that matches existing test conventions.
- Keep test behavior and assertions unchanged.
- Prefer reducing duplication over reorganizing every test.

## Acceptance

- The large test file loses meaningful duplicated setup/click code.
- Tests read more like scenarios and less like plumbing scripts.
- Focused launch/input tests still pass.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_creative_ui_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not weaken assertions just to shrink the file.
- Do not hide setup failures behind helper functions that only return `bool`
  without useful failure text.
- Do not combine with production code changes.

## Completion Brief

Status: done.

Files modified:
- `tests/unit/product_creative_world_launch_tests.cpp`

Changes:
- Added local fixture helpers for default Floor/Wall/Crate/Beam/PointLight
  authored objects.
- Added `BakedCreativeProofObjects` and `createBakedCreativeProofObjects(...)`
  for the repeated Floor/Wall/Crate/Beam/PointLight/PatrolRoute bake proof
  setup.
- Added `installSentinelRoomState(...)` and
  `markCreativeBakedRoomStale(...)` for repeated sentinel/stale setup.
- Replaced repeated setup in open-launch bake, direct refresh, manual rebuild,
  visibility, delete, move, and refresh-failure tests while keeping scenario
  assertions explicit and object-specific.

Behavior:
- No production code changes.
- No assertions were weakened; tests still assert individual create receipts,
  active-room counts, collision counts, stale fields, and command receipts.
- `product_creative_world_launch_tests.cpp` reduced from 3825 to 3790 lines.

Verification:
- `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_creative_ui_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- `rg -n "[ \t]+$" tests/unit/product_creative_world_launch_tests.cpp docs/creative_mode/builder_tasks/claimed/E38-product-creative-world-launch-test-fixtures.md`

Results:
- Build passed.
- Focused CTest passed: 2/2 tests.
- Diff check passed.
- Focused trailing-whitespace scan found no matches.

Concerns:
- This is a conservative local-helper pass. More file-size reduction is
  possible by introducing a broader launch fixture object, but that would touch
  many more scenario bodies and should be a separate cleanup task.
