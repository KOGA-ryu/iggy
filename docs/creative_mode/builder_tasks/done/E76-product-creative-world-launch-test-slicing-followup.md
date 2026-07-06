# E76: Product Creative World Launch Test Slicing Follow-Up

## Objective

Continue splitting `product_creative_world_launch_tests.cpp` so each scenario
proves one behavior instead of one long corridor of unrelated assertions.

## Problem

E63 split the largest generated Room Shell scenario, but the file is still over
4,000 lines and still has many broad tests that mix launch/open/save identity,
RoomBake refresh, active-room/collision state, undo, stale state, projection
counts, and receipt mirrors.

This is the exact kind of green test surface that can hide bad code: a new
feature can append another expectation to a broad scenario and pass without
proving the behavior in isolation.

## Required Reads

- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`
- `docs/creative_mode/builder_tasks/done/E38-product-creative-world-launch-test-fixtures.md`
- `docs/creative_mode/builder_tasks/done/E63-product-creative-integration-scenario-slicing.md`

## Evidence

Current long functions in `product_creative_world_launch_tests.cpp` include:

- `successfulLaunchCreatesSaveSessionInstallsDocumentAndEntersCreativeMode`:
  about 188 lines.
- `autoRefreshMoveCommitAndIgnoresNoChangeReleaseThroughInputFrame`:
  about 186 lines.
- `removeGeneratedRoomShellAndUndoRestoresThroughInputFrame`:
  about 152 lines.
- `secondOpenClearsOldFacadeStateAndInstallsRestoredDocument`:
  about 143 lines.
- `manualRebuildRoomCommandRefreshesBakedActiveRoomThroughInputFrame`:
  about 141 lines.
- More than 20 functions are still over 80 lines.

## Scope

- Split the next highest-value broad scenarios into behavior-focused tests.
- Prefer extracting reusable scenario setup/assertion helpers only when the
  helper returns structured state and keeps failure messages specific.
- Keep real no-window input/command routing proof where that is the behavior
  under test.
- Preserve existing coverage; this is a test-quality repair, not assertion
  deletion.

## Acceptance

- At least two of the remaining over-140-line scenarios are split into smaller
  behavior-focused tests.
- The moved assertions still prove launch/open identity, bake counts, collision,
  stale state, undo, and dirty/save state where applicable.
- Helpers do not hide setup failures behind a single boolean with no context.
- Focused tests still pass.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_creative_ui_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

## Do Not

- Do not weaken or remove behavior assertions to reduce line count.
- Do not replace real routed input proofs with direct helper calls unless the
  scenario under test is not routing.
- Do not combine this with production code changes.

## Completion Brief

Status: done.

Files modified:

- `tests/unit/product_creative_world_launch_tests.cpp`

Scenarios split:

- Split `manualRebuildRoomCommandRefreshesBakedActiveRoomThroughInputFrame` into:
  - `manualRebuildRoomCommandReportsRefreshThroughInputFrame`
  - `manualRebuildRoomCommandLoadsActiveRoomThroughInputFrame`
  - shared structured setup: `ManualRebuildRoomScenario` and
    `runManualRebuildRoomScenario(...)`
- Split `autoRefreshMoveCommitAndIgnoresNoChangeReleaseThroughInputFrame` into:
  - `autoRefreshMoveCommitRefreshesBakedRoomThroughInputFrame`
  - `autoRefreshNoChangeMoveReleaseDoesNotRefreshThroughInputFrame`
  - `undoAfterMoveCommitRestoresBakedRoomThroughInputFrame`
  - shared structured setup/execution: `AutoMoveScenario`,
    `makeAutoMoveScenario(...)`, `runAutoMoveCommit(...)`,
    `runAutoMoveNoChangeRelease(...)`, and `runAutoMoveUndo(...)`

Coverage preserved:

- Manual rebuild still proves routed UI input, command receipt/status, bake
  refresh counts, stale clearing, undo-depth preservation, active-room/collision
  counts, projection roles, creative identity, product-save identity, and dirty
  preservation.
- Move tests still prove real pointer lifecycle routing, revision change,
  same-frame auto-refresh, no-change release with no refresh/no undo push, and
  undo restoring baked geometry through the same input-frame path.
- No production code changed.
- No assertions were removed to make the tests shorter.

Line-count note:

- `product_creative_world_launch_tests.cpp` is now 4182 lines. The split improves
  failure locality but does not reduce total LOC because the setup state is kept
  explicit instead of hidden behind bool-only helpers.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_creative_ui_input_frame_tests -j10`
  passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
  passed: 2/2 tests.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing whitespace scan over
  `tests/unit/product_creative_world_launch_tests.cpp` passed.

Concerns:

- This file still needs a later fixture extraction if LOC reduction is the goal.
  E76 focused on behavior slicing, not moving shared scenario state into a
  separate test-support module.
