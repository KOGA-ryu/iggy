# E216 - Operations Split G4c: Product New World Launch

## Status

Done.

## Context

E210-E215 split the first Operations.cpp seams:

- E210 moved creative baked active-room refresh into `creative/BakedActiveRoomRefresh.*`.
- E211 moved save-slot browser/delete/recover operations into `save/SaveSlotOperations.*`.
- E212 moved creative world launch/open/save operations into `creative/CreativeWorldOperations.*`.
- E213 audited the remaining product session/world launch coupling.
- E214 moved product package-path/world-template resolution into `world/ProductWorldTemplateOperations.*`.
- E215 moved product package/session bootstrap and save/load session launch into `world/ProductSessionLaunch.*`.

`Operations.cpp` now mostly holds product new-world launch, current-session save/write, and the creative blank-stage bridge wrappers used by creative world operations/session launch cleanup. Move only the product new-world/ASCII authoring/initial-save launch seam next.

## Objective

Extract product new-world launch from `Operations.*` into a world-owned helper file.

Add:

- `src/app/iggy3d/world/ProductNewWorldLaunch.hpp`
- `src/app/iggy3d/world/ProductNewWorldLaunch.cpp`

Move from `Operations.*`:

- `launchProductNewWorld(...)`

Move from `Operations.cpp` private helpers:

- `productAsciiRoomAuthoringRequestFromWorldSetup(...)`
- `prepareProductWorldCreationFromDraft(...)`
- `recordProductWorldInitialSaveResult(...)`

Keep `launchProductNewWorld(...)` as the only public API from the new header unless compile fallout proves otherwise.

## Required Behavior Preservation

Preserve current product new-world launch behavior exactly:

- `window.frontendShell.launchAction = "create_and_enter"`
- world template comes from `productWorldTemplateFromOptions(options)`
- world setup diagnostics and route statuses are written exactly as before
- built-in dungeon index/count behavior is unchanged
- `prepareProductWorldCreation(...)` input still uses:
  - routed world setup request
  - `world`
  - `options.saveRoot`
  - `productSaveTimestampNowUtc()`
  - `nextProductWorldId(options.saveRoot)`
- ASCII-room branch behavior is unchanged:
  - authoring request comes from `productWorldSetupAuthoringRequest(...)`
  - preview recording still uses `recordProductAsciiRoomPreview(...)`
  - ASCII package still uses `makeProductAsciiRoomPackage(...)`
  - session bootstrap still goes through `createProductSessionFromPackage(...)`
  - active room still uses `buildProductActiveRoomFromAsciiAuthoring(...)`
  - active-room revision/collision refresh still occur
- non-ASCII branch still goes through `createProductSession(...)`
- initial save behavior is unchanged:
  - `ProductWorldInitialSaveRequest` fields and attempt token stay the same
  - `writeProductWorldInitialSaveDurably(...)` call stays the same
  - producer-side receipt fields are recorded exactly as before
  - failed initial save still calls `clearProductGameplayLaunchState(...)`
- success still sets `window.inputDevice.interactionMode = ProductInteractionMode::Player`
- success still calls `enterProductGameplayTransition(frontend, window, FrontendAction::CreateAndEnter)`

## Expected Source Edits

Likely touched production files:

- `CMakeLists.txt`
- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/world/ProductNewWorldLaunch.hpp`
- `src/app/iggy3d/world/ProductNewWorldLaunch.cpp`
- `src/app/iggy3d/AppKernel.cpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- focused compiler-reported direct users

Likely touched tests:

- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_starter_menu_action_tests.cpp`
- `tests/unit/product_window_input_frame_tests.cpp`
- any compiler-reported direct users of `launchProductNewWorld(...)`

Add `src/app/iggy3d/world/ProductNewWorldLaunch.cpp` to the `iggy3d` library source list near the other `src/app/iggy3d/world/*.cpp` entries.

## Include Policy

- Files that call `launchProductNewWorld(...)` should include `app/iggy3d/world/ProductNewWorldLaunch.hpp`.
- Do not keep `Operations.hpp` includes solely for `launchProductNewWorld(...)`.
- The new implementation may include `app/iggy3d/Operations.hpp` for `clearProductGameplayLaunchState(...)` if needed. Do not move the cleanup wrapper in this slice.

## Non-Scope

Do not move or reshape:

- `writeProductCurrentSessionSave(...)`
- `recordProductSaveWriteResult(...)`
- `createCreativeBlankSession(...)`
- `frameCreativeStageCameraOnOrigin(...)`
- `clearProductGameplayLaunchState(...)`
- product session launch/bootstrap operations
- save/load session launch
- product world-template operations
- save-slot browser/delete/recover operations
- creative world operations

Do not change:

- save catalog selection policy
- save/load format
- package lookup policy
- package/scenario identities
- receipt keys/order/values
- renderer/window/projection behavior
- CMake test definitions

No staging, commit, push, or window launch.

## Required Greps

After implementation, run and report:

```sh
rg -n "launchProductNewWorld|productAsciiRoomAuthoringRequestFromWorldSetup|prepareProductWorldCreationFromDraft|recordProductWorldInitialSaveResult" /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/world/ProductNewWorldLaunch.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/world/ProductNewWorldLaunch.cpp
```

Expected classification:

- `Operations.hpp` has no `launchProductNewWorld(...)` declaration.
- `Operations.cpp` has no product new-world helper definitions.
- New product-new-world files own the declaration/definition for `launchProductNewWorld(...)` and its private helpers.

Also run and report direct include cleanup:

```sh
rg -n "#include \"app/iggy3d/Operations.hpp\"" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
```

Classify remaining `Operations.hpp` include users. Do not widen scope just to chase includes.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_starter_menu_action_tests product_window_input_frame_tests product_new_world_menu_action_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_new_world_menu_action_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- receipt golden output changes
- initial save/catalog behavior changes
- ASCII room preview/session/active-room behavior changes
- package path/status/session timing receipt values change unexpectedly
- the extraction forces current-session save/write or creative blank-stage wrappers into the new file
- compile fallout expands beyond direct include repairs

## Completion Brief

When done, report:

- files changed
- exact APIs moved/added
- where `launchProductNewWorld(...)` now lives
- how the new file calls session bootstrap and cleanup without moving those owners
- remaining `Operations.hpp` include users and why they remain
- required grep classifications
- receipt golden result
- focused build/CTest results
- diff/whitespace checks
- confirmation that current-session save/write and creative blank-stage wrappers were not moved

## Completion Brief

- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/Operations.hpp`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/AppKernel.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/world/ProductNewWorldLaunch.hpp`
  - `src/app/iggy3d/world/ProductNewWorldLaunch.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `tests/unit/product_starter_menu_action_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
  - this task card
- Exact APIs moved/added:
  - Removed from `Operations.hpp` and declared in `world/ProductNewWorldLaunch.hpp`: `launchProductNewWorld(...)`.
  - Moved into `world/ProductNewWorldLaunch.cpp`: `productAsciiRoomAuthoringRequestFromWorldSetup(...)`, `prepareProductWorldCreationFromDraft(...)`, `recordProductWorldInitialSaveResult(...)`, and `launchProductNewWorld(...)`.
  - Added `src/app/iggy3d/world/ProductNewWorldLaunch.cpp` to the `iggy3d` library source list near the other world sources.
- New location:
  - `launchProductNewWorld(...)` now lives in `src/app/iggy3d/world/ProductNewWorldLaunch.cpp`, declared by `src/app/iggy3d/world/ProductNewWorldLaunch.hpp`.
- Owner calls preserved:
  - Session bootstrap remains owned by `world/ProductSessionLaunch.*`; the new file calls `createProductSessionFromPackage(...)` for the ASCII package path and `createProductSession(...)` for the non-ASCII path.
  - Gameplay cleanup remains owned by `Operations.*`; the new file includes `app/iggy3d/Operations.hpp` only to call `clearProductGameplayLaunchState(...)` on failed initial save.
- Remaining `Operations.hpp` include users:
  - Direct Operations-owned API users remain: `src/app/iggy3d/save/Flow.cpp` for `writeProductCurrentSessionSave(...)`; `src/app/iggy3d/creative/CreativeWorldOperations.cpp`, `src/app/iggy3d/world/ProductSessionLaunch.cpp`, and `src/app/iggy3d/world/ProductNewWorldLaunch.cpp` for creative blank-stage / gameplay-launch cleanup wrappers; `src/app/iggy3d/Operations.cpp` for its own declarations.
  - Existing non-E216 include users remain in `src/app/iggy3d/window/InputFrame.cpp` and several product tests; they do not call `launchProductNewWorld(...)` and were not chased because this card explicitly says not to widen into include cleanup.
- Required grep classifications:
  - `Operations.hpp`: no `launchProductNewWorld(...)` declaration remains.
  - `Operations.cpp`: no product new-world helper definitions remain.
  - `ProductNewWorldLaunch.hpp/.cpp`: own the moved public declaration/definition and private helpers.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- Focused build/CTest results:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_starter_menu_action_tests product_window_input_frame_tests product_new_world_menu_action_tests product_receipt_key_order_tests -j10` passed. Build emitted an existing unused-variable warning in `product_starter_menu_action_tests.cpp`.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_new_world_menu_action_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 5/5.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card found no hits.
- Confirmations:
  - Current-session save/write behavior was not moved.
  - Creative blank-stage wrappers were not moved.
  - Product session launch/bootstrap operations, save/load session launch, product world-template operations, save-slot browser/delete/recover operations, creative world operations, save/load format, receipt keys/order/values, CMake test definitions, staging, commit, push, and window launch were not changed.
