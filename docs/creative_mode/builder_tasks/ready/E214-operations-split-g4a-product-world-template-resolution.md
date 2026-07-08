# E214 - Operations Split G4a: Product World Template Resolution

## Status

Ready.

## Context

E210-E212 split the first Operations.cpp seams:

- E210 moved creative baked active-room refresh into `creative/BakedActiveRoomRefresh.*`.
- E211 moved save-slot browser/delete/recover operations into `save/SaveSlotOperations.*`.
- E212 moved creative world launch/open/save operations into `creative/CreativeWorldOperations.*`.
- E213 audited the remaining Operations.cpp seams and recommended this narrow pre-step before product save/load session launch.

`Operations.cpp` still owns both package path resolution and `productWorldTemplateFromOptions(...)`. That function now fans out across app startup, menu actions, save-slot operations, and a direct test. Move only the world-template/path resolution seam first so later session-launch work can consume a smaller API without dragging product new-world or save/load launch behavior into the same slice.

## Objective

Extract product package-path/world-template resolution from `Operations.*` into a world-owned helper file.

Add:

- `src/app/iggy3d/world/ProductWorldTemplateOperations.hpp`
- `src/app/iggy3d/world/ProductWorldTemplateOperations.cpp`

Move/surface:

- `std::filesystem::path productPackagePathFromOptions(const ProductAppOptions& options);`
- `ProductWorldTemplate productWorldTemplateFromOptions(const ProductAppOptions& options);`

Remove `productWorldTemplateFromOptions(...)` from `Operations.hpp/.cpp`.

Move the current file-local `defaultProductPackagePath(...)` behavior into `productPackagePathFromOptions(...)`.

## Required Behavior Preservation

`productPackagePathFromOptions(...)` must preserve the exact current package path policy:

- If `options.devPackageOverride` is not empty, return it.
- Otherwise call `resolvePackageRuntimeLookup(...)` with:
  - `PackageMode::BuildTreeProduct`
  - `requireGraphicsRuntime = false`
  - `requireShaderRoot = false`
- If lookup succeeds and `resourceRoot` is non-empty, return:
  - `<resourceRoot>/demos/first_room/package.iggy3d.toml`
- Otherwise return:
  - `fixtures/demos/first_room/package.iggy3d.toml`

`productWorldTemplateFromOptions(...)` must preserve current behavior:

- Default to `defaultWorldTemplate()`.
- Load the package at `productPackagePathFromOptions(options)`.
- If load succeeds and rooms are available, set the world room dimensions from `package.rooms.front().room.dimensions`.
- Preserve all current status-insensitive fallback behavior.

## Expected Source Edits

Likely touched production files:

- `CMakeLists.txt`
- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/AppKernel.cpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `src/app/iggy3d/save/SaveSlotOperations.cpp`
- `src/app/iggy3d/world/ProductWorldTemplateOperations.hpp`
- `src/app/iggy3d/world/ProductWorldTemplateOperations.cpp`

Likely touched tests:

- `tests/unit/product_window_input_frame_tests.cpp`
- any compiler-reported direct test user of `productWorldTemplateFromOptions(...)`

Add the new `.cpp` to the `iggy3d` library source list near the other `src/app/iggy3d/world/*.cpp` entries.

## Include Policy

- Files that only need template/path resolution should include `app/iggy3d/world/ProductWorldTemplateOperations.hpp`.
- Do not keep `Operations.hpp` includes solely for `productWorldTemplateFromOptions(...)`.
- `Operations.cpp` may include the new header for its remaining `launchProductNewWorld(...)` internal call if needed.

## Non-Scope

Do not move or reshape:

- `createProductSessionFromPackage(...)`
- `createProductSession(...)`
- `launchProductNewWorld(...)`
- `launchProductContinueSave(...)`
- `launchProductLoadSaveSelection(...)`
- `writeProductCurrentSessionSave(...)`
- `createCreativeBlankSession(...)`
- `frameCreativeStageCameraOnOrigin(...)`
- `clearProductGameplayLaunchState(...)`
- save-slot browser/delete/recover operations
- creative world operations

Do not change:

- package lookup policy
- package/scenario identities
- save catalog behavior
- save/load format
- receipt keys/order/values
- renderer/window/projection behavior
- CMake test definitions

No staging, commit, push, or window launch.

## Required Greps

After implementation, run and report:

```sh
rg -n "productWorldTemplateFromOptions|productPackagePathFromOptions|defaultProductPackagePath" /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/world/ProductWorldTemplateOperations.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/world/ProductWorldTemplateOperations.cpp
```

Expected classification:

- `Operations.hpp` has no `productWorldTemplateFromOptions(...)` declaration.
- `Operations.cpp` has no `defaultProductPackagePath(...)` helper definition.
- `Operations.cpp` may retain call sites to `productWorldTemplateFromOptions(...)` only if required by remaining launch behavior.
- New world-template files own the declarations/definitions for `productPackagePathFromOptions(...)` and `productWorldTemplateFromOptions(...)`.

Also run and report direct include cleanup:

```sh
rg -n "#include \"app/iggy3d/Operations.hpp\"" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
```

Classify any remaining `Operations.hpp` include users. Do not widen scope just to chase includes.

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

- package path strings or package/scenario identity output changes
- save catalog behavior changes
- receipt golden output changes
- the extraction forces session bootstrap, save/load launch, or product new-world launch into the new file
- compile fallout expands beyond direct include repairs

## Completion Brief

When done, report:

- files changed
- exact API moved/added
- where `productWorldTemplateFromOptions(...)` now lives
- whether `productPackagePathFromOptions(...)` preserves `devPackageOverride`, build-tree lookup, and fixture fallback
- remaining `Operations.hpp` include users and why they remain
- required grep classifications
- receipt golden result
- focused build/CTest results
- diff/whitespace checks
- confirmation that session launch/new-world/save-write behavior was not moved
