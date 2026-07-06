# E44: Product Creative Bake Refresh Service

## Objective

Extract the product Creative baked-room refresh flow out of `Operations.cpp`
into a small named service/helper so launch/open/manual/auto refresh share a
reviewable seam.

## Problem

`refreshProductCreativeBakedActiveRoom(...)` currently owns all of this in one
operation body: precondition rejection, RoomBake request construction, timing
diagnostics, no-renderable clear/preserve policy, active-room construction,
collision construction, source/count mirroring, and stale/fresh recording.

That made each later feature add another field or branch in the same corridor.
It works, but it is becoming a policy magnet inside `Operations.cpp`, and it is
hard to review whether a new refresh caller is using the same semantics as the
existing launch/open/manual/auto paths.

## Required Reads

- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/creative/adapters/RoomBake.hpp`
- `src/app/iggy3d/gameplay/ActiveRoomState.hpp/.cpp`
- `src/app/iggy3d/gameplay/ActiveRoomCollision.hpp/.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`

## Scope

- Extract the refresh implementation into a focused product/creative helper or
  local service type. Keep the public `refreshProductCreativeBakedActiveRoom(...)`
  API stable unless a smaller API cleanup is clearly mechanical.
- Keep current behavior unchanged:
  - inactive/missing-session/invalid-document fail closed;
  - default no-renderable preserves existing active room/collision;
  - `clearOnNoRenderable=true` clears active room/collision and clears stale;
  - accepted renderable bake replaces active room/collision and clears stale;
  - timing measures only `buildRoomAssetFromCreativeDocument(...)`.
- Keep all existing receipt/result fields and diagnostics.

## Acceptance

- `Operations.cpp` no longer has a long procedural bake-refresh body; it delegates
  to a named seam whose responsibilities are testable/readable.
- Launch/open/manual/auto refresh tests still prove identical accepted,
  rejected, no-renderable clear, no-renderable preserve, and timing behavior.
- Adding a new refresh policy should require editing one small service/helper,
  not a launch/open operation corridor.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_ui_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change RoomBake policy.
- Do not add a bake cache.
- Do not change launch/open/manual/auto refresh semantics.
- Do not route anchors into session seed.

## Completion Brief

- Status: done.
- Files modified:
  - `src/app/iggy3d/Operations.cpp`
- Implementation:
  - Extracted `refreshProductCreativeBakedActiveRoom(...)` into a local
    `ProductCreativeBakedRoomRefreshService`.
  - Kept the public refresh API stable.
  - Preserved current precondition rejection, RoomBake timing scope,
    no-renderable clear/preserve policy, active-room/collision replacement, and
    stale/fresh recording behavior.
  - The public operation now delegates to the named service seam.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_ui_input_frame_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over `src/app/iggy3d/Operations.cpp`
- Result: all passed.
- Note: focused build still prints the existing
  `product_creative_ui_input_frame_tests.cpp:310` missing-field initializer
  warning; this slice did not touch that test.
