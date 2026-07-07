# E148: RoomStore G1 - Accessor Seam

## Objective

Start the structural `activeRoom` regroup safely by adding an accessor seam for the window-owned active-room cluster.

This is a behavior-preserving setup card. It should not move storage yet.

## Context

Ratified preflight: `docs/creative_mode/builder_tasks/done/E140-activeroom-roomstore-preflight.md`.

This is **not** an ownership kill. It is a structural regroup toward the god-struct decomposition map. The actual staleness/ownership bug was already handled by the active-room collision freshness store.

## Required Work

1. Add a small RoomStore accessor seam. Suggested home:
   - `src/app/iggy3d/gameplay/ProductRoomStore.hpp`
   - `.cpp` only if definitions require it.

2. The seam should expose named accessors for the current window-owned cluster:
   - `activeRoom(window)`
   - `activeRoomRevision(window)`
   - `activeRoomCollision(window)`
   - `activeRoomCollisionFreshness(window)`
   - const overloads where needed.

3. Initially the accessors should forward to the existing top-level fields:
   - `window.activeRoom`
   - `window.activeRoomRevision`
   - `window.activeRoomCollision`
   - `window.activeRoomCollisionFreshness`

4. Add narrow compile/behavior tests proving:
   - accessors return references to the current top-level fields;
   - writing through non-const accessors changes the same state;
   - const accessors expose the same values.

5. Do not migrate broad call sites yet. One or two local tests using the helper are fine, but production migration starts in E149.

## Do Not

- Do not add `ProductRoomStore` storage yet.
- Do not remove or rename current top-level fields.
- Do not touch `roomEditing.activeRoom` or `roomEditing.activeRoomCollision`.
- Do not change receipt keys or regenerate receipt golden.
- Do not change collision freshness semantics.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_active_room_state_tests product_active_room_collision_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_active_room_state_tests|product_active_room_collision_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Accessor seam added:
- Storage unchanged proof:
- Tests/checks run:
- Concerns/deferred:
