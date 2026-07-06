# E92: Flow Pause Save Overload Trim

## Objective

Trim the current pause-save overload set down by removing verified dead/test-only
overloads while preserving save and creative identity behavior.

## Problem

`executeProductPauseSaveFlow(...)` currently has extra overload bodies in
`Flow.cpp` / `Flow.hpp`:

- The 6-arg `FrontendSettings&` overload is dead, with zero repo callers.
- The 6-arg `creative::CreativeAppState*` overload is test-only, with one caller
  in `product_creative_world_launch_tests.cpp`.

The public entry should keep the settings+creative-app path where both concerns
are needed, and the plain product path for callers that do not need either.

## Scope

- Delete the dead 6-arg settings overload and declaration.
- Migrate the lone test-only 6-arg creative-app caller to the 7-arg
  settings+creative-app overload by passing a local `FrontendSettings`.
- Delete the 6-arg creative-app overload and declaration.
- Keep pause save, save-and-exit, creative save, active creative identity, and
  return-to-title behavior unchanged.

## Do Not

- Do not touch SaveFileStore twins.
- Do not change save codec behavior.
- Do not change product live UI, RoomBake, creative command routing, or window
  launch behavior.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `src/app/iggy3d/save/Flow.hpp`
- `src/app/iggy3d/save/Flow.cpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`

## Acceptance

- Repo search shows no remaining callers or declarations for the removed
  overloads.
- `product_creative_world_launch_tests` passes.
- Save round-trip focused tests are unchanged/green if touched by the overload
  cut.

## Suggested Verification

```sh
rg -n "executeProductPauseSaveFlow\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/save/Flow.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/save/Flow.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/menu/ActionHandlers.cpp /Users/kogaryu/iggy3d/tests/unit/product_creative_world_launch_tests.cpp
cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_save_bridge_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_save_bridge_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files modified:
- Overloads removed:
- Caller migration:
- Verification:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files modified:
  - `src/app/iggy3d/save/Flow.hpp`
  - `src/app/iggy3d/save/Flow.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/claimed/E92-flow-pause-save-overload-trim.md`
- Overloads removed:
  - Removed the 6-arg `executeProductPauseSaveFlow(..., CreativeAppState*)`
    declaration/body.
  - Removed the 6-arg `executeProductPauseSaveFlow(..., FrontendSettings&)`
    declaration/body.
  - Remaining pause-save entry points are the plain product path and the
    settings+CreativeApp path.
- Caller migration:
  - Migrated the stale creative mirror regression in
    `product_creative_world_launch_tests.cpp` to pass a local
    `FrontendSettings` and call the settings+CreativeApp overload.
- Verification:
  - `rg -n "executeProductPauseSaveFlow\\(" ...` shows only two declarations,
    two bodies, the internal plain-product delegation, the two pause-menu
    callers, and the migrated test caller.
  - `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_world_launch_tests product_save_bridge_tests -j10`
    passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_save_bridge_tests)$' --output-on-failure`
    passed.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing whitespace scan over touched files passed.
- Concerns/deferred:
  - No SaveFileStore files touched.
  - No commit/stage/push performed.
