# E145: CreativeFly G5 - Seeder And Integrator Migration

## Objective

Migrate the remaining creative-fly seeders and integrator writes to the store helpers, preserving their distinct behaviors.

After this gate, production should not write `creativeFlyAnchorValid` or `creativeFlyPositionMeters` directly outside the store compatibility layer.

## Prerequisite

E142-E144 complete.

## Required Work

1. Migrate `src/app/iggy3d/window/InputFrame.cpp`:
   - Replace local `ensureCreativeFlyAnchor(...)` raw-field writes with `ensureFreshCreativeFlyAnchor(...)`.
   - Preserve both callers:
     - map-maker fly path;
     - creative navigate path that currently passes `&*context.activeSession`.
   - Preserve the navigate-active precondition before dereferencing the optional session.

2. Migrate the creative fly integrator block:
   - Continue writing `creativeFlyActive`, `creativeFlyStatus`, `creativeFlyReasonCode`, and `creativeFlySpeedMetersPerSecond` in `window.viewport`.
   - Route final anchor updates through `recordCreativeFlyAnchorIntegrated(...)`.

3. Migrate `src/app/iggy3d/gameplay/ProjectionRefresh.cpp::mapMakerAnchorFor(...)`:
   - If the store is fresh, return the store position.
   - If scene player anchor exists, seed through scene provenance and return it.
   - If no player anchor exists, return `{}` without latching, preserving current retry-next-frame behavior.

4. Keep legacy raw fields mirrored by helper calls until G7.

5. Add/update tests proving:
   - map-maker movement still activates creative fly and advances the anchor.
   - no-player map-maker projection does not latch an anchor.
   - creative navigate caller still seeds safely through the helper.
   - production direct assignment grep for raw anchor fields is empty outside the store helper and tests.

## Do Not

- Do not delete legacy raw fields yet.
- Do not change receipt fields or regenerate receipt golden.
- Do not move `creativeFlyActive` or sibling status/speed fields.
- Do not touch standalone app fly state.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Grep

Report this grep and justify any non-test production survivor:

```sh
rg -n "creativeFlyAnchorValid\\s*=|creativeFlyPositionMeters\\s*=" /Users/kogaryu/iggy3d/src/app/iggy3d --glob '*.cpp' --glob '*.hpp'
```

Expected production survivors after this card should be limited to the store helper compatibility layer.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_creative_navigate_fly_tests product_vulkan_room_frame_tests product_creative_world_launch_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_creative_navigate_fly_tests|product_vulkan_room_frame_tests|product_creative_world_launch_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- InputFrame seeder migration:
- Integrator migration:
- mapMakerAnchorFor behavior preserved:
- Direct raw-field writer grep:
- Tests/checks run:
- Concerns/deferred:

## Completed Brief

- Files changed:
  - Updated `src/app/iggy3d/window/InputFrame.cpp`.
  - Updated `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`.
  - Updated `tests/unit/product_window_input_frame_tests.cpp`.
  - Updated `tests/unit/product_vulkan_room_frame_tests.cpp`.
- InputFrame seeder migration:
  - Removed the local `activePlayerPositionOrOrigin(...)` and
    `ensureCreativeFlyAnchor(...)` raw-field writer helpers from
    `InputFrame.cpp`.
  - `applyProductWindowCreativeFlyActions(...)` now starts from
    `ensureFreshCreativeFlyAnchor(window, activeSession)`.
  - The creative Navigate path now calls
    `ensureFreshCreativeFlyAnchor(context.window, &*context.activeSession)`
    after the existing Navigate-active guard.
- Integrator migration:
  - `creativeFlyActive`, `creativeFlyStatus`, `creativeFlyReasonCode`, and
    `creativeFlySpeedMetersPerSecond` remain in `window.viewport`.
  - Applied fly movement now records the final anchor via
    `recordCreativeFlyAnchorIntegrated(...)`, which mirrors legacy fields for
    compatibility.
- mapMakerAnchorFor behavior preserved:
  - Fresh store anchors are returned directly.
  - Scene player anchors seed through `seedCreativeFlyAnchorFromScene(...)` and
    are returned.
  - No-player scene projection returns `{}` without latching store or legacy
    anchor fields, preserving retry-next-frame behavior.
- Direct raw-field writer grep:
  - `rg -n "creativeFlyAnchorValid\\s*=|creativeFlyPositionMeters\\s*=" /Users/kogaryu/iggy3d/src/app/iggy3d --glob '*.cpp' --glob '*.hpp'`
    reports only:
    - `src/app/iggy3d/view/CreativeFlyAnchorStore.cpp` compatibility mirror
      writes.
    - `src/app/iggy3d/view/ViewportState.hpp` legacy field initializer, which
      remains until G7.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_creative_navigate_fly_tests product_vulkan_room_frame_tests product_creative_world_launch_tests -j10`
    passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_creative_navigate_fly_tests|product_vulkan_room_frame_tests|product_creative_world_launch_tests)$' --output-on-failure`
    passed: 4/4.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched/new files passed.
- Concerns/deferred:
  - Legacy raw fields and receipt migration remain for G7.
  - G6 still needs stress ordering coverage for epoch/open/reseed behavior.
  - `Testing/Temporary/LastTest.log` remains dirty from CTest output and was
    intentionally left untouched.
