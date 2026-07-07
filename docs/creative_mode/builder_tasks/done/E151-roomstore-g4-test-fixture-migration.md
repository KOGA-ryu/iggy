# E151: RoomStore G4 - Test Fixture Migration

## Objective

Migrate tests and fixtures off direct `window.activeRoom*` access so the final storage move can be compiler-small.

This is expected to be the churniest card. Keep it mechanical and behavior-preserving.

## Prerequisite

E148-E150 complete.

## Required Work

1. Migrate test references for the window-owned active-room cluster to the accessor seam:
   - `window.activeRoom`
   - `window.activeRoomRevision`
   - `window.activeRoomCollision`
   - `window.activeRoomCollisionFreshness`

2. Keep `roomEditing.activeRoom` test references unchanged when they are asserting editor producer state.

3. Prefer local helper functions in tests where it reduces repetition, but do not build a new test framework.

4. Keep behavior assertions unchanged:
   - active-room load/status/counts;
   - collision query counts;
   - freshness receipt reasons;
   - revision bumps.

## Required Greps

Report:

```sh
rg -n "window\\.activeRoom\\b|window\\.activeRoomRevision\\b|window\\.activeRoomCollision\\b|window\\.activeRoomCollisionFreshness\\b" /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
rg -n "roomEditing\\.activeRoom\\b|roomEditing\\.activeRoomCollision\\b" /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
```

Expected:
- first grep should be empty or every survivor justified as awaiting E152;
- second grep may remain where tests intentionally inspect room-editor producer state.

## Do Not

- Do not move storage yet.
- Do not change production behavior.
- Do not weaken assertions just to get green.
- Do not touch unrelated tests.
- Do not stage, commit, push, launch a window, or run broad CTest unless focused fallout is too broad.

## Suggested Verification

Run the focused targets for touched tests. At minimum include:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_active_room_collision_tests product_active_room_state_tests product_creative_no_window_bake_scenario_tests product_creative_world_launch_tests product_gameplay_tape_runner_tests product_window_input_frame_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_active_room_collision_tests|product_active_room_state_tests|product_creative_no_window_bake_scenario_tests|product_creative_world_launch_tests|product_gameplay_tape_runner_tests|product_window_input_frame_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Test migration shape:
- Required grep results:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - E151

- Files changed:
  - `tests/unit/product_active_room_collision_tests.cpp`
  - `tests/unit/product_active_room_state_tests.cpp`
  - `tests/unit/product_ascii_room_activation_tests.cpp`
  - `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`
  - `tests/unit/product_creative_ui_input_frame_tests.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `tests/unit/product_gameplay_controller_tests.cpp`
  - `tests/unit/product_gameplay_tape_runner_tests.cpp`
  - `tests/unit/product_interaction_mode_state_tests.cpp`
  - `tests/unit/product_new_world_menu_action_tests.cpp`
  - `tests/unit/product_vulkan_room_frame_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
  - `docs/creative_mode/builder_tasks/claimed/E151-roomstore-g4-test-fixture-migration.md`
- Test migration shape:
  - Product window active-room fixture reads/writes now use
    `iggy3d::activeRoom(...)`, `iggy3d::activeRoomRevision(...)`,
    `iggy3d::activeRoomCollision(...)`, and
    `iggy3d::activeRoomCollisionFreshness(...)`.
  - The mechanical migration covered `window`, `openWindow`,
    `scenario.window`, and `request.window` receivers where they are
    `ProductAppWindowState` instances.
  - `product_active_room_state_tests.cpp` keeps direct field references only in
    the E148 alias/write-through proof that intentionally verifies storage has
    not moved yet.
- Required grep results:
  - `rg -n "window\\.activeRoom\\b|window\\.activeRoomRevision\\b|window\\.activeRoomCollision\\b|window\\.activeRoomCollisionFreshness\\b" /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'`
  - First grep now reports only the deliberate
    `product_active_room_state_tests.cpp` E148 alias/write-through storage proof
    awaiting E152.
  - `rg -n "roomEditing\\.activeRoom\\b|roomEditing\\.activeRoomCollision\\b" /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'`
  - Second grep returns no matches in tests after the migration.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_active_room_collision_tests product_active_room_state_tests product_creative_no_window_bake_scenario_tests product_creative_world_launch_tests product_gameplay_tape_runner_tests product_window_input_frame_tests product_receipt_key_order_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_active_room_collision_tests|product_active_room_state_tests|product_creative_no_window_bake_scenario_tests|product_creative_world_launch_tests|product_gameplay_tape_runner_tests|product_window_input_frame_tests|product_receipt_key_order_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over touched/new files.
- Concerns/deferred:
  - E152 should remove or rewrite the direct E148 top-level-field alias proof as
    part of the actual storage move.
