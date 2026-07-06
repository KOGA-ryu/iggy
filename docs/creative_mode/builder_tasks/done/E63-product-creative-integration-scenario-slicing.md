# E63: Product Creative Integration Scenario Slicing

## Objective

Split oversized product Creative integration tests into smaller behavior-focused
scenarios so green tests prove the intended behavior rather than merely pinning
one long scripted corridor.

## Problem

`product_creative_world_launch_tests.cpp` now contains several very large
no-window scenarios that combine setup, command routing, bake refresh, active
room assertions, undo behavior, and unrelated rejection behavior in one test
function. That makes failures hard to diagnose and encourages adding more
assertions to an already broad test instead of creating a narrow proof.

This is the test version of file bloat: it can keep the build green while
making the code harder to change because every feature is coupled to a giant
end-to-end script.

## Required Reads

- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`
- `docs/creative_mode/builder_tasks/ready/E38-product-creative-world-launch-test-fixtures.md`

## Evidence

Current largest test functions in `product_creative_world_launch_tests.cpp`:

- `generateRoomShellFromSelectedRoomBakesAndUndoClearsThroughInputFrame`:
  `2296-2606`, 311 lines. It covers Create Room, Generate Shell, RoomBake,
  parent-delete rejection, Undo, active-room clear, stale state, and dirty flags.
- `autoRefreshMoveCommitAndIgnoresNoChangeReleaseThroughInputFrame`:
  `3321-3509`, 189 lines.
- `successfulLaunchCreatesSaveSessionInstallsDocumentAndEntersCreativeMode`:
  `408-592`, 185 lines.
- Several other scenarios exceed 100 lines and mix launch/open/save identity
  with bake/receipt assertions.

## Scope

- After or alongside E38 fixture extraction, split the largest scenarios into
  smaller tests by behavior:
  - launch/open identity,
  - manual rebuild,
  - auto-refresh,
  - delete,
  - undo,
  - generated room shell,
  - parent-delete rejection.
- Keep end-to-end no-window coverage where it proves real routing, but avoid
  combining unrelated behavior in a single test function.
- Prefer named scenario helpers that return useful structured state over
  boolean-only scripts with hundreds of chained `expect(...)` calls.

## Acceptance

- No single product Creative world-launch test function remains responsible for
  proving more than one user-visible behavior and its immediate side effects.
- The generated Room Shell proof is split so parent-delete rejection and Undo
  behavior are separate from shell generation and bake refresh.
- Failure messages remain specific; do not hide setup failures inside helpers.
- Focused tests still prove the same behavior and keep current routing coverage.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_creative_ui_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not remove no-window routing proof.
- Do not weaken assertions to make tests shorter.
- Do not combine this with production routing changes.

## Completion Brief

- Files modified:
  - `tests/unit/product_creative_world_launch_tests.cpp`
- Implementation:
  - Split the oversized generated Room Shell integration scenario into a
    structured setup helper plus three behavior-focused tests:
    - `generateRoomShellFromSelectedRoomBakesThroughInputFrame()`
    - `generatedRoomShellParentDeleteRejectsThroughInputFrame()`
    - `removeGeneratedRoomShellAndUndoRestoresThroughInputFrame()`
  - Kept the real no-window UI/input routing proof for Create Room, Generate
    Room Shell, parent Delete Selected rejection, Remove Room Shell, Undo, and
    RoomBake auto-refresh.
  - Preserved explicit assertions and failure messages for shell counts, bake
    counts, active-room/collision state, undo depth, stale state, identity, and
    dirty flags.
- Verification:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing whitespace scan over
    `tests/unit/product_creative_world_launch_tests.cpp` passed.
  - `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_creative_ui_input_frame_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_input_frame_tests)$' --output-on-failure` passed.
- Concerns:
  - This slices the largest generated-shell scenario only. Other long launch
    tests remain good candidates for later test-only cleanup cards.
