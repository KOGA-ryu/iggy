# E212 — Operations Split G3: Creative World Operations

## Status

Done.

## Context

E209 preflighted the `Operations.cpp` seam split. E210 extracted creative baked
active-room refresh, and E211 extracted save-slot browser/delete/recover
operations. The next seam is creative world lifecycle: create/open/save world
operations plus the result/request structs and private mirroring/status helpers.

This is an extraction-only card. Preserve behavior, receipt fields, receipt
keys/order/values, status strings, and save/load formats.

## Objective

Create a creative-owned operations seam for creative world launch/open/save.

Suggested new files:

- `src/app/iggy3d/creative/CreativeWorldOperations.hpp`
- `src/app/iggy3d/creative/CreativeWorldOperations.cpp`

Add the new `.cpp` to the `iggy3d` library source list in `CMakeLists.txt` near
the other creative app sources.

## Move Scope

Move these public declarations out of `src/app/iggy3d/Operations.hpp` and into
the new creative header:

- `struct ProductCreativeNewWorldLaunchRequest`
- `struct ProductCreativeNewWorldLaunchResult`
- `struct ProductCreativeOpenWorldLaunchRequest`
- `struct ProductCreativeOpenWorldLaunchResult`
- `struct ProductCreativeCurrentWorldSaveResult`
- `launchProductCreativeNewWorld(...)`
- `launchProductCreativeOpenWorld(...)`
- `saveProductCurrentCreativeWorld(...)`

Move the private helpers used by those operations into the new `.cpp`:

- `setCreativeNewWorldLaunchStatus(...)`
- `setCreativeOpenWorldLaunchStatus(...)`
- `setCurrentCreativeSaveStatus(...)`
- `idOrNone(...)`
- `pathOrNone(...)`
- `missingWindowIdentity(...)`
- `recordActiveCreativeSaveIdentity(...)`
- `recordActiveCreativeSaveResult(...)`
- `mirrorCreativeWorldCreateResult(...)`
- `mirrorCreativeWorldOpenResult(...)`
- `mirrorCreativeDocumentInstallResult(...)`
- `mirrorCreativeBakedActiveRoomRefreshResult(...)`

The moved implementation may continue to call existing seams:

- `createCreativeBlankSession(...)`
- `frameCreativeStageCameraOnOrigin(...)`
- `clearProductGameplayLaunchState(...)`
- `enterProductGameplayTransition(...)`
- `refreshProductCreativeBakedActiveRoom(...)`
- `productSaveTimestampNowUtc()`
- `createCreativeWorld(...)`, `openCreativeWorld(...)`,
  `saveCreativeWorld(...)`

Keep those as narrow calls/includes. Do not move their implementations in this
card.

## Explicit Non-Scope

Do not move:

- `productWorldTemplateFromOptions(...)`
- `writeProductCurrentSessionSave(...)`
- `launchProductNewWorld(...)`
- `launchProductContinueSave(...)`
- `launchProductLoadSaveSelection(...)`
- `launchProductSaveSlot(...)`
- package/session bootstrap helpers
- product new-world/ascii launch helpers
- save-slot operations moved in E211
- baked active-room refresh implementation moved in E210
- save/load durable file formats or parser/writer code
- renderer/Vulkan/projection/window-loop code
- `ProductAppWindowState` storage

Do not rename creative launch/save status or reason strings.

## Expected Callers To Update

Likely production callers that should include the new creative header instead
of relying on `Operations.hpp` for these APIs/types:

- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `src/app/iggy3d/save/Flow.hpp`
- `src/app/iggy3d/save/Flow.cpp`

Likely focused tests:

- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`
- any compiler-reported creative UI/window tests that directly use creative
  launch/open/save result or request types

Do not update unrelated includes unless the build proves they are required.

## Required Greps

After the move:

```sh
rg -n "ProductCreativeNewWorldLaunch|ProductCreativeOpenWorldLaunch|ProductCreativeCurrentWorldSave|launchProductCreativeNewWorld|launchProductCreativeOpenWorld|saveProductCurrentCreativeWorld|setCreativeNewWorldLaunchStatus|setCreativeOpenWorldLaunchStatus|setCurrentCreativeSaveStatus|recordActiveCreativeSaveIdentity|recordActiveCreativeSaveResult|mirrorCreativeWorldCreateResult|mirrorCreativeWorldOpenResult|mirrorCreativeDocumentInstallResult|mirrorCreativeBakedActiveRoomRefreshResult" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/creative/CreativeWorldOperations.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/creative/CreativeWorldOperations.cpp
```

Expected result:

- no declarations for moved creative world APIs/types remain in
  `Operations.hpp`;
- no definitions for moved creative world APIs/helpers remain in
  `Operations.cpp`;
- the new creative files contain the moved declarations/definitions.

Also run a focused caller grep and classify remaining hits:

```sh
rg -n "launchProductCreativeNewWorld|launchProductCreativeOpenWorld|saveProductCurrentCreativeWorld|ProductCreativeNewWorldLaunch|ProductCreativeOpenWorldLaunch|ProductCreativeCurrentWorldSave" \
  /Users/kogaryu/iggy3d/src/app/iggy3d \
  /Users/kogaryu/iggy3d/tests/unit \
  --glob '*.cpp' --glob '*.hpp'
```

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_no_window_bake_scenario_tests product_starter_menu_action_tests product_window_input_frame_tests product_creative_ui_input_frame_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_creative_ui_input_frame_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening if:

- the move forces product new-world/ascii launch, package/session bootstrap, or
  save-slot operations into the new creative file;
- a circular include appears that cannot be fixed by moving includes to `.cpp`
  files;
- receipt golden output changes;
- any creative launch/open/save status or reason string changes;
- creative world launch/open/save tests show identity, undo, active-room refresh,
  or blank-stage behavior changed.

## Completion Brief Required

Report:

- new header/source paths;
- exact public APIs/types moved from `Operations.hpp`;
- exact private helpers moved;
- remaining `Operations.cpp` hits for creative world APIs/helpers and their
  classification;
- caller include updates;
- receipt golden result;
- focused build/CTest result;
- `git diff --check` and trailing-whitespace result;
- confirmation that product new-world/load, package/session bootstrap,
  save-slot operations, baked-room refresh implementation, save formats,
  receipt keys, staging, commit, push, and window launch were not changed.

## Completion Brief

- Card moved to done: yes.
- New header/source paths:
  - `src/app/iggy3d/creative/CreativeWorldOperations.hpp`
  - `src/app/iggy3d/creative/CreativeWorldOperations.cpp`
- Public APIs/types moved from `Operations.hpp`:
  - `ProductCreativeNewWorldLaunchRequest`
  - `ProductCreativeNewWorldLaunchResult`
  - `ProductCreativeOpenWorldLaunchRequest`
  - `ProductCreativeOpenWorldLaunchResult`
  - `ProductCreativeCurrentWorldSaveResult`
  - `launchProductCreativeNewWorld(...)`
  - `launchProductCreativeOpenWorld(...)`
  - `saveProductCurrentCreativeWorld(...)`
- Private helpers moved from `Operations.cpp`:
  - `setCreativeNewWorldLaunchStatus(...)`
  - `setCreativeOpenWorldLaunchStatus(...)`
  - `setCurrentCreativeSaveStatus(...)`
  - `idOrNone(...)`
  - `pathOrNone(...)`
  - `missingWindowIdentity(...)`
  - `recordActiveCreativeSaveIdentity(...)`
  - `recordActiveCreativeSaveResult(...)`
  - `mirrorCreativeWorldCreateResult(...)`
  - `mirrorCreativeWorldOpenResult(...)`
  - `mirrorCreativeDocumentInstallResult(...)` overloads
  - `mirrorCreativeBakedActiveRoomRefreshResult(...)` overloads
- Narrow Operations seam kept in place:
  - `createCreativeBlankSession(...)`
  - `frameCreativeStageCameraOnOrigin(...)`
  - `clearProductGameplayLaunchState(...)`
  - Their implementations remain in `Operations.cpp` as file-local `*Impl`
    helpers behind public wrappers so the new creative seam can call them
    without moving product new-world/session launch implementation.
- Remaining `Operations.cpp` hits for creative world APIs/helpers:
  - Required grep found no moved creative world API/type/helper declarations or
    definitions in `Operations.hpp` or `Operations.cpp`.
  - `Operations.cpp` only keeps the narrow blank-stage/gameplay-launch helper
    wrappers listed above; those are not creative world API/helper names from
    the E212 move list.
- Caller include updates:
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/save/Flow.hpp`
  - `src/app/iggy3d/save/Flow.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`
- CMake update:
  - Added `src/app/iggy3d/creative/CreativeWorldOperations.cpp` to the
    `iggy3d` library source list near other creative app sources.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`
    produced no diff.
- Focused build:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_no_window_bake_scenario_tests product_starter_menu_action_tests product_window_input_frame_tests product_creative_ui_input_frame_tests product_receipt_key_order_tests -j10`
    passed.
  - The build emitted an existing warning in
    `tests/unit/product_starter_menu_action_tests.cpp` for an unused local
    `facade`; it was not part of this extraction.
- Focused CTest:
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_creative_ui_input_frame_tests|product_receipt_key_order_tests)$' --output-on-failure`
    passed: 6/6.
- Diff and whitespace:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card produced
    no output.
- Confirmation:
  - Product new-world/load, package/session bootstrap, save-slot operations,
    baked-room refresh implementation, save formats, receipt keys, CMake test
    definitions, staging, commit, push, and window launch were not changed.
