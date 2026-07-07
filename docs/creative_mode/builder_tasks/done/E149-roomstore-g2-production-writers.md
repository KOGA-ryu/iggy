# E149: RoomStore G2 - Production Writers

## Objective

Migrate production active-room writers and freshness-store internals to the RoomStore accessor seam from E148.

This should still be behavior-preserving. Storage remains top-level until E152.

## Prerequisite

E148 complete.

If the accessor seam is missing, move this card to `blocked/` with evidence.

## Required Work

1. Migrate production writer sites that assign the window-owned active-room cluster:
   - `src/app/iggy3d/Operations.cpp`
   - `src/app/iggy3d/ascii_room/Activation.cpp`
   - `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
   - `src/app/iggy3d/gameplay/TapeRunner.cpp`
   - `src/app/iggy3d/gameplay/ActiveRoomState.cpp`
   - `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.cpp`

2. Keep writer behavior unchanged:
   - every active-room publication still bumps revision once;
   - collision freshness store still stamps/updates the collision result exactly as before;
   - no direct rebake path is reintroduced.

3. Do not migrate broad read-only sites unless required by local compilation.

4. Preserve `roomEditing.activeRoom` and `roomEditing.activeRoomCollision` exactly; those are producer state, not window truth.

## Required Grep

Report:

```sh
rg -n "window\\.activeRoom\\s*=|window\\.activeRoomCollision\\s*=|window\\.activeRoomRevision\\b|window\\.activeRoomCollisionFreshness\\s*=" /Users/kogaryu/iggy3d/src/app/iggy3d --glob '*.cpp' --glob '*.hpp'
```

Expected production survivors after this card:
- reader sites not yet migrated;
- top-level field declarations;
- any unavoidable compatibility accessors.

No production assignment to the cluster should remain outside the RoomStore accessor/helper seam, unless justified.

## Do Not

- Do not move storage yet.
- Do not migrate tests broadly.
- Do not touch `roomEditing.activeRoom` or `roomEditing.activeRoomCollision`.
- Do not change receipt keys or regenerate receipt golden.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_active_room_collision_tests product_active_room_state_tests product_gameplay_tape_runner_tests product_creative_no_window_bake_scenario_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_active_room_collision_tests|product_active_room_state_tests|product_gameplay_tape_runner_tests|product_creative_no_window_bake_scenario_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Writer sites migrated:
- Revision/freshness behavior preserved:
- Required grep result:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - E149

- Files changed:
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/ascii_room/Activation.cpp`
  - `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
  - `src/app/iggy3d/gameplay/TapeRunner.cpp`
  - `src/app/iggy3d/gameplay/ActiveRoomState.cpp`
  - `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `docs/creative_mode/builder_tasks/claimed/E149-roomstore-g2-production-writers.md`
- Writer sites migrated:
  - Startup/package session active-room publication and clears now write through
    `activeRoom(window)` / `activeRoomCollision(window)`.
  - Creative baked-room refresh install and clear paths now publish through the
    RoomStore accessors.
  - ASCII activation, room-editing automation mirror, saved authored-room load,
    tape-runner temporary collision freshness, and gameplay collision freshness
    recording now use the accessor seam.
  - `bumpActiveRoomRevision(...)` now increments `activeRoomRevision(window)`.
  - `ensureActiveRoomCollisionFresh(...)` now reads/writes active-room state and
    collision through the accessor seam.
- Revision/freshness behavior preserved:
  - Existing publication sites still call `bumpActiveRoomRevision(window)` once
    at the same points as before.
  - Collision freshness still stamps `bakedFromRoomRevision` and
    `bakedFromSessionHash`, and writes the rebuilt collision state once through
    `activeRoomCollision(window)`.
  - `roomEditing.activeRoom` and `roomEditing.activeRoomCollision` producer
    state was not migrated or renamed.
- Required grep result:
  - `rg -n "window\\.activeRoom\\s*=|window\\.activeRoomCollision\\s*=|window\\.activeRoomRevision\\b|window\\.activeRoomCollisionFreshness\\s*=" /Users/kogaryu/iggy3d/src/app/iggy3d --glob '*.cpp' --glob '*.hpp'`
  - Remaining matches are only in `src/app/iggy3d/gameplay/ProductRoomStore.hpp`
    where the accessors forward to `window.activeRoomRevision`.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_active_room_collision_tests product_active_room_state_tests product_gameplay_tape_runner_tests product_creative_no_window_bake_scenario_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_active_room_collision_tests|product_active_room_state_tests|product_gameplay_tape_runner_tests|product_creative_no_window_bake_scenario_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over touched/new files.
- Concerns/deferred:
  - Production readers are intentionally not swept here. E150 can migrate
    read-only and receipt paths through the same seam.
