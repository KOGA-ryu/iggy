# E218 - Operations Split G4e: Remove Operations Shell

## Status

Done.

## Context

E210-E217 split the first Operations.cpp seams:

- E210 moved creative baked active-room refresh into `creative/BakedActiveRoomRefresh.*`.
- E211 moved save-slot browser/delete/recover operations into `save/SaveSlotOperations.*`.
- E212 moved creative world launch/open/save operations into `creative/CreativeWorldOperations.*`.
- E213 audited remaining product session/world launch coupling.
- E214 moved product package-path/world-template resolution into `world/ProductWorldTemplateOperations.*`.
- E215 moved product package/session bootstrap and save/load session launch into `world/ProductSessionLaunch.*`.
- E216 moved product new-world launch into `world/ProductNewWorldLaunch.*`.
- E217 moved current-session save/write into `save/CurrentSessionSave.*`.

`Operations.cpp` now contains only the creative blank-stage session helpers and a generic gameplay launch cleanup helper. Finish the split by moving those helpers to explicit owners and deleting the empty `Operations.*` shell.

## Objective

Remove the remaining `Operations.*` API surface.

Add:

- `src/app/iggy3d/creative/CreativeBlankStageSession.hpp`
- `src/app/iggy3d/creative/CreativeBlankStageSession.cpp`
- `src/app/iggy3d/world/ProductLaunchState.hpp`
- `src/app/iggy3d/world/ProductLaunchState.cpp`

Move:

- `createCreativeBlankSession(...)` to `creative/CreativeBlankStageSession.*`
- `frameCreativeStageCameraOnOrigin(...)` to `creative/CreativeBlankStageSession.*`
- `clearProductGameplayLaunchState(...)` to `world/ProductLaunchState.*`

Delete if no callers remain:

- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/Operations.cpp`

Remove `src/app/iggy3d/Operations.cpp` from the `iggy3d` library source list.

## Required Behavior Preservation

Preserve creative blank-stage behavior exactly:

- startup package path remains `"creative_blank_stage"`
- package lookup/load timing/status fields stay the same
- package load status remains `"ok"`
- creative blank session still seeds one local player at origin
- player local bounds stay `{-0.25, 0.0, -0.25}` to `{0.25, 1.8, 0.25}`
- inert objective id/status/condition/player slot stay unchanged
- package id remains `"iggy3d.creative_blank"`
- session create failure still writes launch/status timing fields exactly as before
- success still clears active room and active-room collision, bumps active-room revision, stores the session, sets gameplay runtime flags, and sets launch status `"runtime_session_created"`

Preserve creative stage camera behavior exactly:

- bump creative world epoch
- seed creative fly anchor from origin
- yaw `0.0F`
- pitch `-30.0F`

Preserve gameplay launch cleanup exactly:

- `window.gameplay.gameplayActive = false`
- `window.gameplay.runtimeSessionCreated = false`
- clear active room and active-room collision
- bump active-room revision
- reset active session

## Expected Source Edits

Likely touched production files:

- `CMakeLists.txt`
- `src/app/iggy3d/Operations.hpp` (delete)
- `src/app/iggy3d/Operations.cpp` (delete)
- `src/app/iggy3d/creative/CreativeBlankStageSession.hpp`
- `src/app/iggy3d/creative/CreativeBlankStageSession.cpp`
- `src/app/iggy3d/world/ProductLaunchState.hpp`
- `src/app/iggy3d/world/ProductLaunchState.cpp`
- `src/app/iggy3d/creative/CreativeWorldOperations.cpp`
- `src/app/iggy3d/world/ProductSessionLaunch.cpp`
- `src/app/iggy3d/world/ProductNewWorldLaunch.cpp`
- focused compiler-reported direct users

Likely touched tests:

- compiler-reported files that still include `app/iggy3d/Operations.hpp`

Add the new `.cpp` files to the `iggy3d` library source list near their owner groups.

## Include Policy

- Files that call `createCreativeBlankSession(...)` or `frameCreativeStageCameraOnOrigin(...)` should include `app/iggy3d/creative/CreativeBlankStageSession.hpp`.
- Files that call `clearProductGameplayLaunchState(...)` should include `app/iggy3d/world/ProductLaunchState.hpp`.
- No file should include `app/iggy3d/Operations.hpp` after this slice.

## Non-Scope

Do not move or reshape:

- creative world launch/open/save operations
- product new-world launch
- product session launch/bootstrap operations
- save/load session launch
- current-session save/write
- save-slot browser/delete/recover operations
- product world-template operations

Do not change:

- save/load durable format
- package/session bootstrap behavior
- creative world launch/open/save behavior
- receipt keys/order/values
- renderer/window/projection behavior
- CMake test definitions

No staging, commit, push, or window launch.

## Required Greps

After implementation, run and report:

```sh
rg -n "createCreativeBlankSession|frameCreativeStageCameraOnOrigin|clearProductGameplayLaunchState" /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/CreativeBlankStageSession.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/CreativeBlankStageSession.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/world/ProductLaunchState.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/world/ProductLaunchState.cpp
```

Expected classification:

- `Operations.hpp/.cpp` are deleted or have no matches.
- creative blank-stage files own `createCreativeBlankSession(...)` and `frameCreativeStageCameraOnOrigin(...)`.
- product launch-state files own `clearProductGameplayLaunchState(...)`.

Also run:

```sh
rg -n "#include \"app/iggy3d/Operations.hpp\"|Operations\\.cpp|Operations\\.hpp" /Users/kogaryu/iggy3d/CMakeLists.txt /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp' --glob 'CMakeLists.txt'
```

Expected result:

- no active source/test/CMake references to `Operations.hpp` or `Operations.cpp`

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_no_window_bake_scenario_tests product_starter_menu_action_tests product_window_input_frame_tests product_save_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_save_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- receipt golden output changes
- creative blank-world launch behavior changes
- gameplay launch cleanup behavior changes
- deleting `Operations.*` causes broad include fallout beyond direct replacements
- the move forces any already-extracted operation back into a combined helper

## Completion Brief

When done, report:

- files changed/deleted
- exact APIs moved and their new owners
- whether `Operations.hpp/.cpp` were deleted
- final `Operations.hpp`/`Operations.cpp`/include/CMake grep result
- receipt golden result
- focused build/CTest results
- diff/whitespace checks
- confirmation that previously extracted operations were not moved again

## Completion Brief

- Files changed/deleted:
  - `CMakeLists.txt`
  - deleted `src/app/iggy3d/Operations.hpp`
  - deleted `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/creative/CreativeBlankStageSession.hpp`
  - `src/app/iggy3d/creative/CreativeBlankStageSession.cpp`
  - `src/app/iggy3d/world/ProductLaunchState.hpp`
  - `src/app/iggy3d/world/ProductLaunchState.cpp`
  - `src/app/iggy3d/creative/CreativeWorldOperations.cpp`
  - `src/app/iggy3d/world/ProductSessionLaunch.cpp`
  - `src/app/iggy3d/world/ProductNewWorldLaunch.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/window/Loop.cpp`
  - `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`
  - `tests/unit/product_creative_ui_frame_tests.cpp`
  - `tests/unit/product_creative_ui_input_frame_tests.cpp`
  - `tests/unit/product_creative_viewport_pick_frame_tests.cpp`
  - `tests/unit/product_frontend_router_tests.cpp`
  - `tests/unit/product_save_delete_executor_tests.cpp`
  - this task card
- Exact APIs moved and new owners:
  - `createCreativeBlankSession(...)` moved to `src/app/iggy3d/creative/CreativeBlankStageSession.cpp`, declared by `CreativeBlankStageSession.hpp`.
  - `frameCreativeStageCameraOnOrigin(...)` moved to `src/app/iggy3d/creative/CreativeBlankStageSession.cpp`, declared by `CreativeBlankStageSession.hpp`.
  - `clearProductGameplayLaunchState(...)` moved to `src/app/iggy3d/world/ProductLaunchState.cpp`, declared by `ProductLaunchState.hpp`.
- `Operations.hpp/.cpp` deletion:
  - Both files were deleted.
  - `src/app/iggy3d/Operations.cpp` was removed from the `iggy3d` library source list.
  - Added `src/app/iggy3d/creative/CreativeBlankStageSession.cpp` near the creative sources.
  - Added `src/app/iggy3d/world/ProductLaunchState.cpp` near the world sources.
- Include/caller repair:
  - `CreativeWorldOperations.cpp` now includes `creative/CreativeBlankStageSession.hpp` and `world/ProductLaunchState.hpp`.
  - `ProductSessionLaunch.cpp` and `ProductNewWorldLaunch.cpp` now include `world/ProductLaunchState.hpp`.
  - Stale `Operations.hpp` includes were removed from `InputFrame.cpp` and the compiler-covered product tests.
  - Two stale comments naming `Operations.cpp` were updated to current owner names.
- Final grep result:
  - Symbol owner grep shows only `CreativeBlankStageSession.hpp/.cpp` for `createCreativeBlankSession(...)` and `frameCreativeStageCameraOnOrigin(...)`, and only `ProductLaunchState.hpp/.cpp` for `clearProductGameplayLaunchState(...)`.
  - Exact deleted-shell grep for `app/iggy3d/Operations.hpp`, `app/iggy3d/Operations.cpp`, and `#include "app/iggy3d/Operations.hpp"` across CMake, `src/app/iggy3d`, and `tests/unit` returned no hits.
  - The card's broad `Operations\\.cpp|Operations\\.hpp` pattern still matches surviving owner filenames such as `CreativeWorldOperations.cpp`, `SaveSlotOperations.cpp`, and `ProductWorldTemplateOperations.cpp`; these are not references to the deleted `Operations.*` shell.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- Focused build/CTest results:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_no_window_bake_scenario_tests product_starter_menu_action_tests product_window_input_frame_tests product_save_bridge_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_save_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 6/6.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card found no hits.
- Confirmations:
  - Previously extracted operations were not moved again.
  - Creative world launch/open/save operations, product new-world launch, product session launch/bootstrap operations, save/load session launch, current-session save/write, save-slot browser/delete/recover operations, product world-template operations, save/load durable format, package/session bootstrap behavior, receipt keys/order/values, CMake test definitions, staging, commit, push, and window launch were not changed.
