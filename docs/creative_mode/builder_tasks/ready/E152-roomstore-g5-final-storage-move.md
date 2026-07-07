# E152: RoomStore G5 - Final Storage Move

## Objective

Move the window-owned active-room cluster into a `ProductRoomStore` sub-struct and delete the old top-level fields.

This is the delete-last gate. It should reduce the `ProductAppWindowState` top-level member count without changing behavior.

## Prerequisite

E148-E151 complete.

## Required Work

1. Add real `ProductRoomStore` storage containing:
   - `ProductActiveRoomState activeRoom`
   - `std::uint64_t activeRoomRevision = 0`
   - `ProductActiveRoomCollisionState activeRoomCollision`
   - `ProductActiveRoomCollisionFreshnessResult activeRoomCollisionFreshness`

2. Add `ProductRoomStore room;` to `ProductAppWindowState`.

3. Delete old top-level fields from `ProductAppWindowState`:
   - `activeRoom`
   - `activeRoomRevision`
   - `activeRoomCollision`
   - `activeRoomCollisionFreshness`

4. Update the E148 accessors to return `window.room.*`.

5. Update `docs/god_struct_member_ownership.tsv`:
   - remove rows for the four old top-level fields;
   - add a row for `room` assigned to `RoomStore`.

6. Regenerate or update ownership coverage as needed.

7. Receipt golden should stay unchanged. Run the receipt oracle; regenerate only if the key-order test says it is intentionally required.

## Required Negative Greps

Report:

```sh
rg -n "window\\.activeRoom\\b|window\\.activeRoomRevision\\b|window\\.activeRoomCollision\\b|window\\.activeRoomCollisionFreshness\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
rg -n "ProductAppWindowState.*activeRoom|activeRoomRevision|activeRoomCollision|activeRoomCollisionFreshness" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp
rg -n "roomEditing\\.activeRoom\\b|roomEditing\\.activeRoomCollision\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
```

Expected:
- first grep: no direct window top-level refs;
- second grep: no old top-level declarations, but accessors/store types may have names elsewhere;
- third grep: still has intentional room-editor producer-state references.

## Do Not

- Do not delete or migrate `roomEditing.activeRoom` or `roomEditing.activeRoomCollision`.
- Do not change active-room/collision semantics.
- Do not change receipt keys.
- Do not do unrelated ProductAppWindowState decomposition.
- Do not stage, commit, push, or launch a window.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
./build/product_receipt_key_order_tests
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_god_struct_ownership_coverage_tests|product_receipt_key_order_tests|product_active_room_collision_tests|product_active_room_state_tests|product_creative_no_window_bake_scenario_tests|product_creative_world_launch_tests|product_gameplay_tape_runner_tests|product_window_input_frame_tests|product_room_editing_state_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Store storage moved:
- Top-level fields deleted:
- Ownership TSV change:
- Receipt oracle result:
- Negative grep results:
- Tests/checks run:
- Concerns/deferred:
