# E150: RoomStore G3 - Production Readers And Receipts

## Objective

Migrate production read sites and active-room receipt appender code to the RoomStore accessor seam.

This card should leave tests mostly untouched except for local compile fallout. Storage still stays top-level until E152.

## Prerequisite

E148-E149 complete.

## Required Work

1. Migrate production readers of the window-owned cluster:
   - `src/app/iggy3d/receipt/ActiveRoomFields.cpp`
   - `src/app/iggy3d/window/InputFrame.cpp`
   - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
   - `src/app/iggy3d/gameplay/Controller.cpp`
   - `src/app/iggy3d/gameplay/ScriptedDriver.cpp`
   - `src/app/iggy3d/automation/AutomationGameplay.cpp`
   - `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
   - `src/app/iggy3d/menu/ActionHandlers.cpp`
   - any other production reader found by grep.

2. Keep `WorldAuthoringFields.cpp` reads of `window.roomEditing.activeRoom` and `window.roomEditing.activeRoomCollision` unchanged. Those are a different receipt question.

3. Receipt key order and values must stay unchanged.

4. Do not migrate tests broadly in this card unless needed for compile.

## Required Greps

Report:

```sh
rg -n "window\\.activeRoom\\b|window\\.activeRoomCollision\\b|window\\.activeRoomRevision\\b|window\\.activeRoomCollisionFreshness\\b" /Users/kogaryu/iggy3d/src/app/iggy3d --glob '*.cpp' --glob '*.hpp'
rg -n "roomEditing\\.activeRoom\\b|roomEditing\\.activeRoomCollision\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt /Users/kogaryu/iggy3d/src/app/iggy3d/room_editor --glob '*.cpp' --glob '*.hpp'
```

Expected:
- first grep should be down to declarations/accessor seam only, or every survivor justified;
- second grep should still show the room-editor producer/receipt reads.

## Do Not

- Do not move storage yet.
- Do not change receipt keys or regenerate receipt golden unless the oracle demands it.
- Do not touch or delete `roomEditing.activeRoom`.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests product_active_room_collision_tests product_creative_no_window_bake_scenario_tests product_window_input_frame_tests product_gameplay_tape_runner_tests -j10
./build/product_receipt_key_order_tests
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_active_room_collision_tests|product_creative_no_window_bake_scenario_tests|product_window_input_frame_tests|product_gameplay_tape_runner_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Production readers migrated:
- Receipt oracle result:
- Required grep results:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - E150

- Files changed:
  - `src/app/iggy3d/receipt/ActiveRoomFields.cpp`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/ascii_room/Activation.cpp`
  - `src/app/iggy3d/automation/AutomationGameplay.cpp`
  - `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `src/app/iggy3d/gameplay/ScriptedDriver.cpp`
  - `src/app/iggy3d/gameplay/TapeRunner.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `docs/creative_mode/builder_tasks/claimed/E150-roomstore-g3-production-readers-and-receipts.md`
- Production readers migrated:
  - Active-room receipt appender now reads `activeRoom(window)`,
    `activeRoomCollision(window)`, and `activeRoomCollisionFreshness(window)`.
  - Gameplay projection, controller, scripted-driver, automation, menu edit-room,
    ASCII activation, tape-runner, save-writing, and creative baked-room
    activation-hook reads now go through the RoomStore accessor seam.
  - `WorldAuthoringFields.cpp` `window.roomEditing.activeRoom` and
    `window.roomEditing.activeRoomCollision` reads were intentionally left
    unchanged.
- Receipt oracle result:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`
  - `receipt key-order oracle: 1032 fields match golden (order + values)`
- Required grep results:
  - `rg -n "window\\.activeRoom\\b|window\\.activeRoomCollision\\b|window\\.activeRoomRevision\\b|window\\.activeRoomCollisionFreshness\\b" /Users/kogaryu/iggy3d/src/app/iggy3d --glob '*.cpp' --glob '*.hpp'`
  - First grep now reports only `src/app/iggy3d/gameplay/ProductRoomStore.hpp`,
    where the accessors forward to current top-level storage.
  - `rg -n "roomEditing\\.activeRoom\\b|roomEditing\\.activeRoomCollision\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt /Users/kogaryu/iggy3d/src/app/iggy3d/room_editor --glob '*.cpp' --glob '*.hpp'`
  - Second grep still reports the intended `WorldAuthoringFields.cpp`
    `window.roomEditing.activeRoom` and
    `window.roomEditing.activeRoomCollision` receipt reads.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests product_active_room_collision_tests product_creative_no_window_bake_scenario_tests product_window_input_frame_tests product_gameplay_tape_runner_tests -j10`
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_active_room_collision_tests|product_creative_no_window_bake_scenario_tests|product_window_input_frame_tests|product_gameplay_tape_runner_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over touched/new files.
- Concerns/deferred:
  - Storage remains top-level by design. E151 can migrate test fixtures/local
    expectations, and E152 can perform the actual storage regroup.
