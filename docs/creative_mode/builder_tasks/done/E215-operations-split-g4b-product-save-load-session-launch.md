# E215 - Operations Split G4b: Product Save/Load Session Launch

## Status

Done.

## Context

E210-E214 split the first Operations.cpp seams:

- E210 moved creative baked active-room refresh into `creative/BakedActiveRoomRefresh.*`.
- E211 moved save-slot browser/delete/recover operations into `save/SaveSlotOperations.*`.
- E212 moved creative world launch/open/save operations into `creative/CreativeWorldOperations.*`.
- E213 audited the remaining product session/world launch coupling.
- E214 moved product package-path/world-template resolution into `world/ProductWorldTemplateOperations.*`.

The next cohesive seam is save/load session launch. `Operations.cpp` still owns package-to-session bootstrap plus continue/load-save launch. That cluster is separate from product new-world ASCII authoring and separate from current-session save/write.

## Objective

Extract product package/session bootstrap and save/load session launch from `Operations.*` into a new world/session-owned helper file.

Add:

- `src/app/iggy3d/world/ProductSessionLaunch.hpp`
- `src/app/iggy3d/world/ProductSessionLaunch.cpp`

Move from `Operations.*`:

- `launchProductContinueSave(...)`
- `launchProductLoadSaveSelection(...)`

Move from `Operations.cpp` private helpers if needed by the save/load launch seam:

- `packageLoadStatusName(...)`
- `elapsedMicroseconds(...)`
- `createProductSessionFromPackage(...)`
- `createProductSession(...)`
- `recordProductSaveLoadSelection(...)`
- `recordProductSaveLoadResult(...)`
- `recordSavedRoomMarkerBindingResult(...)`
- `saveSlotById(...)`
- `launchProductSaveSlot(...)`

The generic session bootstrap helpers may need public declarations because `launchProductNewWorld(...)` still uses package/session bootstrap for product new-world and ASCII room launch. Keep those declarations narrow and do not move the product new-world launch implementation in this slice.

## Suggested Public API Shape

In `ProductSessionLaunch.hpp`, expose only the functions needed outside the new implementation file:

```cpp
bool createProductSessionFromPackage(const PackageLoadResult& package,
                                     std::optional<Session>& activeSession,
                                     ProductAppWindowState& window);

bool createProductSession(const ProductAppOptions& options,
                          std::optional<Session>& activeSession,
                          ProductAppWindowState& window);

void launchProductContinueSave(const ProductAppOptions& options,
                               const ProductWorldTemplate& world,
                               const ProductSaveBridgeResult& saves,
                               FrontendState& frontend,
                               std::optional<Session>& activeSession,
                               ProductAppWindowState& window);

void launchProductLoadSaveSelection(const ProductAppOptions& options,
                                    const ProductWorldTemplate& world,
                                    const ProductSaveBridgeResult& saves,
                                    FrontendState& frontend,
                                    std::optional<Session>& activeSession,
                                    ProductAppWindowState& window);
```

Use forward declarations where practical. Include complete types in the `.cpp`.

`launchProductSaveSlot(...)` should stay file-local in the new `.cpp` unless the compiler proves an outside caller needs it.

## Required Behavior Preservation

Preserve the current package/session bootstrap behavior exactly:

- package load status string mapping
- startup package path/lookup timing fields
- package load timing/status fields
- runtime session create timing/status fields
- active-room and active-room-collision reset
- package room to active-room projection
- active-room revision bump
- `ensureActiveRoomCollisionFresh(...)`
- `window.gameplay.runtimeSessionCreated`
- `window.gameplay.gameplayActive`
- `window.frontendShell.launchStatus`

Preserve the current save/load launch behavior exactly:

- Continue uses `selectProductContinueSave(...)` and then resolves the matching slot by id.
- Load-save selection sets `frontend.selectedAction = FrontendAction::Load`.
- Load-save selection initializes the selected save slot with `initializeSelectedProductSaveSlot(...)`.
- Slot null/disabled/incompatible status/reason/frontend status behavior is unchanged.
- `ProductSaveLoadRequest` uses the selected slot path, active session, and expected package/scenario ids from `ProductWorldTemplate`.
- Save load result recording still normalizes empty record id to `"none"`.
- Saved authored-room load still builds active room, bumps active-room revision, binds room markers, records bind result, refreshes collision, and handles bind failure through gameplay cleanup.
- Successful load still sets `window.inputDevice.interactionMode = ProductInteractionMode::Player` and enters gameplay transition with the requested launch action.

## Expected Source Edits

Likely touched production files:

- `CMakeLists.txt`
- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/world/ProductSessionLaunch.hpp`
- `src/app/iggy3d/world/ProductSessionLaunch.cpp`
- `src/app/iggy3d/AppKernel.cpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- focused compiler-reported direct users

Likely touched tests:

- `tests/unit/product_starter_menu_action_tests.cpp`
- `tests/unit/product_window_input_frame_tests.cpp`
- `tests/unit/product_save_delete_executor_tests.cpp`
- any compiler-reported direct users of the moved APIs

Add `src/app/iggy3d/world/ProductSessionLaunch.cpp` to the `iggy3d` library source list near the other `src/app/iggy3d/world/*.cpp` entries.

## Include Policy

- Files that call `launchProductContinueSave(...)`, `launchProductLoadSaveSelection(...)`, or session bootstrap helpers should include `app/iggy3d/world/ProductSessionLaunch.hpp`.
- Do not keep `Operations.hpp` includes solely for save/load launch APIs.
- `Operations.cpp` should include the new header only for remaining product new-world calls to `createProductSession(...)` / `createProductSessionFromPackage(...)`.

## Non-Scope

Do not move or reshape:

- `launchProductNewWorld(...)`
- `prepareProductWorldCreationFromDraft(...)`
- `productAsciiRoomAuthoringRequestFromWorldSetup(...)`
- `recordProductWorldInitialSaveResult(...)`
- `writeProductCurrentSessionSave(...)`
- `recordProductSaveWriteResult(...)`
- `createCreativeBlankSession(...)`
- `frameCreativeStageCameraOnOrigin(...)`
- `clearProductGameplayLaunchState(...)` except as a called dependency
- save-slot browser/delete/recover operations
- product world-template operations
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
rg -n "launchProductContinueSave|launchProductLoadSaveSelection|launchProductSaveSlot|createProductSessionFromPackage|createProductSession\\(|recordProductSaveLoadSelection|recordProductSaveLoadResult|recordSavedRoomMarkerBindingResult|saveSlotById|packageLoadStatusName|elapsedMicroseconds" /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/world/ProductSessionLaunch.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/world/ProductSessionLaunch.cpp
```

Expected classification:

- `Operations.hpp` has no `launchProductContinueSave(...)` or `launchProductLoadSaveSelection(...)` declarations.
- `Operations.cpp` has no save/load launch helper definitions.
- `Operations.cpp` may retain calls to `createProductSession(...)` / `createProductSessionFromPackage(...)` from `launchProductNewWorld(...)`.
- New session-launch files own the declarations/definitions for moved save/load launch and bootstrap helpers.

Also run and report direct include cleanup:

```sh
rg -n "#include \"app/iggy3d/Operations.hpp\"" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
```

Classify remaining `Operations.hpp` include users. Do not widen scope just to chase includes.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_starter_menu_action_tests product_window_input_frame_tests product_save_delete_executor_tests product_automation_dispatch_tests product_creative_world_launch_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_starter_menu_action_tests|product_window_input_frame_tests|product_save_delete_executor_tests|product_automation_dispatch_tests|product_creative_world_launch_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- save catalog behavior changes
- receipt golden output changes
- saved authored-room marker binding behavior changes
- package path/status/session timing receipt values change unexpectedly
- the extraction forces product new-world creation, current-session save/write, or creative blank-stage wrappers into the new file
- compile fallout expands beyond direct include repairs

## Completion Brief

When done, report:

- files changed
- exact APIs moved/added
- where `launchProductContinueSave(...)` and `launchProductLoadSaveSelection(...)` now live
- whether `createProductSession(...)` / `createProductSessionFromPackage(...)` moved and how product new-world still calls them
- remaining `Operations.hpp` include users and why they remain
- required grep classifications
- receipt golden result
- focused build/CTest results
- diff/whitespace checks
- confirmation that product new-world and current-session save/write behavior were not moved

## Completion Brief

- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/Operations.hpp`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/world/ProductSessionLaunch.hpp`
  - `src/app/iggy3d/world/ProductSessionLaunch.cpp`
  - this task card
- Exact APIs moved/added:
  - Removed from `Operations.hpp` and declared in `world/ProductSessionLaunch.hpp`: `launchProductContinueSave(...)`, `launchProductLoadSaveSelection(...)`.
  - Added narrow public bootstrap declarations in `world/ProductSessionLaunch.hpp`: `createProductSessionFromPackage(...)`, `createProductSession(...)`.
  - Moved into `world/ProductSessionLaunch.cpp`: `packageLoadStatusName(...)`, session-launch copy of `elapsedMicroseconds(...)`, `createProductSessionFromPackage(...)`, `createProductSession(...)`, `recordProductSaveLoadSelection(...)`, `recordProductSaveLoadResult(...)`, `recordSavedRoomMarkerBindingResult(...)`, `saveSlotById(...)`, file-local `launchProductSaveSlot(...)`, `launchProductContinueSave(...)`, `launchProductLoadSaveSelection(...)`.
- New location:
  - `launchProductContinueSave(...)` and `launchProductLoadSaveSelection(...)` now live in `src/app/iggy3d/world/ProductSessionLaunch.cpp`, declared by `src/app/iggy3d/world/ProductSessionLaunch.hpp`.
- Product new-world bootstrap:
  - `createProductSession(...)` and `createProductSessionFromPackage(...)` moved to `ProductSessionLaunch`.
  - `Operations.cpp::launchProductNewWorld(...)` still owns product new-world/ASCII authoring and calls the moved bootstrap through `app/iggy3d/world/ProductSessionLaunch.hpp`.
- Remaining `Operations.hpp` include users:
  - Direct Operations-owned API users remain: `AppKernel.cpp`, `ActionHandlers.cpp`, `product_creative_world_launch_tests.cpp`, `product_starter_menu_action_tests.cpp`, and `product_window_input_frame_tests.cpp` for `launchProductNewWorld(...)`; `save/Flow.cpp` for `writeProductCurrentSessionSave(...)`; `CreativeWorldOperations.cpp` and `ProductSessionLaunch.cpp` for creative blank-stage / gameplay-launch cleanup wrappers.
  - Other existing product test/window includes were not chased because this card explicitly limited include cleanup to direct compile fallout.
- Required grep classifications:
  - `Operations.hpp`: no `launchProductContinueSave(...)` or `launchProductLoadSaveSelection(...)` declarations remain.
  - `Operations.cpp`: no save/load launch helper definitions remain; it retains calls to `createProductSessionFromPackage(...)` / `createProductSession(...)` from `launchProductNewWorld(...)`; it also retains its file-local `elapsedMicroseconds(...)` for the creative blank-stage timing path.
  - `ProductSessionLaunch.hpp/.cpp`: own the moved public declarations/definitions and save/load private helpers.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- Focused build/CTest results:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_starter_menu_action_tests product_window_input_frame_tests product_save_delete_executor_tests product_automation_dispatch_tests product_creative_world_launch_tests product_receipt_key_order_tests -j10` passed. Build emitted an existing unused-variable warning in `product_starter_menu_action_tests.cpp`.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_starter_menu_action_tests|product_window_input_frame_tests|product_save_delete_executor_tests|product_automation_dispatch_tests|product_creative_world_launch_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 6/6.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card found no hits.
- Confirmations:
  - Product new-world creation and ASCII authoring behavior were not moved.
  - Current-session save/write behavior was not moved.
  - Save-slot browser/delete/recover operations, product world-template operations, creative world operations, save format, receipt keys/order/values, CMake test definitions, staging, commit, push, and window launch were not changed.
