# E97: Baked Room Activation Session Hook

## Objective

Provide a non-const Session or explicit reasoning-graph hook at the baked-room
activation site so runtime graph fill can run when a CreativeDocument room is
installed into product active-room state.

## Problem

`Operations.cpp` installs a baked `RoomAsset` in the RoomBake refresh path, but
the helper currently holds `activeSession_` as `const std::optional<Session>&`.
The game-side reasoning-graph fill wire needs a non-const session or explicit
`setReasoningGraph` hook where the baked room and creative document context are
available.

This card should add the hook seam only. It should not implement the reasoning
graph kernel.

## Scope

- Inspect `ProductCreativeBakedActiveRoomRefreshExecutor::installBakedRoom(...)`
  and its caller-owned session lifetime.
- Change the smallest operation/helper signature needed to make a non-const
  `Session` or explicit graph-install hook available at the baked-room
  activation site.
- Preserve existing active-room/collision refresh behavior.
- Add focused no-window tests proving the hook seam can mutate session-owned
  derived state without changing launch/open/manual/auto refresh semantics.

## Do Not

- Do not implement `buildReasoningGraph`.
- Do not implement `traversalLinksFromDocument`.
- Do not add patrol waypoint export here.
- Do not change RoomBake policy.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/runtime/session/Session.hpp`
- existing runtime/session graph APIs, especially any `setReasoningGraph` seam.
- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`

## Acceptance

- Baked-room refresh has a deliberate session mutation/hook seam.
- Existing activeRoom and activeRoomCollision behavior is unchanged.
- Tests prove launch/open/manual/auto refresh still pass and the new seam is
  reachable in no-window product tests.
- The card completion brief states exactly where downstream game code should
  attach `buildReasoningGraph -> setReasoningGraph`.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_creative_ui_input_frame_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_input_frame_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files modified:
  - `src/app/iggy3d/ProductCreativeBakedRoomRefresh.hpp`
  - `src/app/iggy3d/Operations.hpp`
  - `src/app/iggy3d/Operations.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Hook/session ownership decision:
  - Changed `refreshProductCreativeBakedActiveRoom(...)` to receive
    `std::optional<Session>&` instead of a const session reference.
  - Added optional `ProductCreativeBakedRoomActivationHook` on
    `ProductCreativeBakedActiveRoomRefreshRequest`.
  - The hook is request-owned, defaults empty, and does not alter launch/open,
    manual rebuild, or auto-refresh behavior unless a caller opts in.
- Downstream attach point:
  - Attach downstream `buildReasoningGraph -> session.setReasoningGraph(...)`
    at `ProductCreativeBakedRoomRefreshService::installBakedRoom(...)`, after
    active room/collision state is installed and before the result is finalized.
  - The hook receives `Session&`, the activated `RoomAsset`, and the source
    `CreativeDocument`.
- Tests:
  - Extended `refreshCreativeBakedActiveRoomBuildsRoomCollisionAndProjection()`
    to provide an activation hook that writes a sentinel `ReasoningGraph` into
    the mutable session through `Session::setReasoningGraph(...)`.
  - The same test asserts the hook saw the launched document id, the baked
    static mesh count, and the session-owned graph node after refresh.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_creative_ui_input_frame_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing whitespace scan over touched E97 files.
  - All passed.
- Concerns/deferred:
  - This only provides the activation seam. It intentionally does not implement
    `buildReasoningGraph`, traversal-link export, patrol waypoint export, or
    any default graph population.
