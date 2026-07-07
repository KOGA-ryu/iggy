# E146: CreativeFly G6 - Stress And Ordering Guards

## Objective

Add focused stress/order tests around the creative-fly anchor store before deleting legacy fields and changing receipts.

No production behavior should change in this card unless a test exposes a real wiring bug in E142-E145.

## Prerequisite

E142-E145 complete.

## Required Work

1. Add the cross-world regression from E141:
   - launch/open world A;
   - seed or integrate the creative fly anchor;
   - launch/open world B;
   - assert `creativeWorldEpoch` changes and the anchor is fresh for B;
   - assert the test does not rely on `Session::stateHash()` or `runtimeStateHash`.

2. Add ordering guards:
   - origin-frame seed wins for a new epoch;
   - lazy session seeder does not overwrite a fresh origin seed in the same epoch;
   - integration updates provenance to `FlyIntegrated`;
   - next world open makes the integrated anchor stale and reseeds.

3. Add no-player scene guard:
   - `mapMakerAnchorFor(...)` no-player branch returns `{}` and keeps store unseeded or unchanged for the current policy.

4. Add idempotence guard:
   - repeated ensure calls in the same epoch do not change position/provenance unexpectedly.

## Do Not

- Do not change receipt fields or regenerate receipt golden.
- Do not delete legacy raw fields yet.
- Do not move `creativeFlyActive` or sibling status/speed fields.
- Do not touch standalone app fly state.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_window_input_frame_tests product_vulkan_room_frame_tests product_creative_fly_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_window_input_frame_tests|product_vulkan_room_frame_tests|product_creative_fly_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Cross-world result:
- Ordering/idempotence guards:
- No-player scene guard:
- Tests/checks run:
- Concerns/deferred:

## Completed Brief

- Files changed:
  - Updated `tests/unit/product_creative_fly_tests.cpp`.
  - Updated `tests/unit/product_creative_world_launch_tests.cpp`.
  - Reused the no-player scene guard added in
    `tests/unit/product_vulkan_room_frame_tests.cpp`.
- Cross-world result:
  - Added a same-window blank world A -> blank world B launch regression.
  - The test integrates the world-A fly anchor, launches world B, and verifies
    `creativeWorldEpoch` increments while the second anchor is origin-framed and
    fresh for the new epoch.
  - It asserts the blank runtime hash remains unchanged, proving freshness does
    not rely on `Session::stateHash()` / `runtimeStateHash` drift.
- Ordering/idempotence guards:
  - Origin seed wins before lazy ensure in the same epoch.
  - Integration changes provenance to `FlyIntegrated`.
  - The next origin-frame seed after an epoch bump resets position/provenance to
    `OriginFramed`.
  - Repeated `ensureFreshCreativeFlyAnchor(...)` calls in one epoch keep
    position/provenance stable.
- No-player scene guard:
  - `mapMakerFrameWithoutPlayerDoesNotLatchFlyAnchor()` exercises the projection
    path with no active session/player scene item and verifies no camera
    override, no legacy latch, and no store seed.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_window_input_frame_tests product_vulkan_room_frame_tests product_creative_fly_tests -j10`
    passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_window_input_frame_tests|product_vulkan_room_frame_tests|product_creative_fly_tests)$' --output-on-failure`
    passed: 4/4.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched/new files passed.
- Concerns/deferred:
  - Receipt fields still expose legacy anchor data until G7.
  - Legacy raw field deletion remains for G7.
  - `Testing/Temporary/LastTest.log` remains dirty from CTest output and was
    intentionally left untouched.
