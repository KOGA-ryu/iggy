# E144: CreativeFly G4 - Origin Seed And Camera Consumer

## Objective

Wire the first real consumer: creative world origin framing and projection camera override should use the `CreativeFlyAnchorStore` freshness model.

This gate gives the store a production path without migrating every seeder yet.

## Prerequisite

E142 and E143 complete.

## Required Work

1. In `src/app/iggy3d/Operations.cpp`, update `frameCreativeStageCameraOnOrigin(...)`:
   - bump `window.creativeWorldEpoch` exactly once per call;
   - call `seedCreativeFlyAnchorFromOrigin(window)`;
   - keep `cameraYawDegrees = 0` and `cameraPitchDegrees = -30` in the caller.

2. In `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`, update the camera anchor override consumer:
   - `frame.cameraAnchorOverrideAvailable` should use the store freshness predicate for `window.creativeWorldEpoch`.
   - `frame.cameraAnchorOverrideMeters` should read the store anchor position.
   - Do not change downstream `ViewportFraming` or `FramePresenter` fan-out.

3. Keep legacy fields mirrored by helper calls until G7 so existing receipt fields still work.

4. Migrate focused tests that directly fabricate/read the raw fields:
   - `tests/unit/product_vulkan_room_frame_tests.cpp`
   - `tests/unit/product_creative_world_launch_tests.cpp`

5. Add or update tests proving:
   - creative launch frames origin anchor through the store;
   - `creativeWorldEpoch` increments on creative new/open origin framing;
   - yaw/pitch behavior remains unchanged;
   - camera override uses the store when fresh.

## Do Not

- Do not migrate `InputFrame.cpp` seeders in this card.
- Do not migrate `mapMakerAnchorFor(...)` in this card.
- Do not delete legacy raw fields.
- Do not change receipt fields or regenerate receipt golden.
- Do not touch standalone app fly state.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_vulkan_room_frame_tests product_creative_fly_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_vulkan_room_frame_tests|product_creative_fly_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Origin seed wiring:
- Camera consumer wiring:
- Legacy field compatibility:
- Tests/checks run:
- Concerns/deferred:
