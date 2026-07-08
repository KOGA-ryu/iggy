# E217 - Operations Split G4d: Current Session Save

## Status

Done.

## Context

E210-E216 split the first Operations.cpp seams:

- E210 moved creative baked active-room refresh into `creative/BakedActiveRoomRefresh.*`.
- E211 moved save-slot browser/delete/recover operations into `save/SaveSlotOperations.*`.
- E212 moved creative world launch/open/save operations into `creative/CreativeWorldOperations.*`.
- E213 audited remaining product session/world launch coupling.
- E214 moved product package-path/world-template resolution into `world/ProductWorldTemplateOperations.*`.
- E215 moved product package/session bootstrap and save/load session launch into `world/ProductSessionLaunch.*`.
- E216 moved product new-world launch into `world/ProductNewWorldLaunch.*`.

`Operations.cpp` now holds current-session save/write plus the creative blank-stage bridge wrappers used by creative world operations and session-launch cleanup. Move only current-session save/write next.

## Objective

Extract current-session save/write from `Operations.*` into a save-owned helper file.

Add:

- `src/app/iggy3d/save/CurrentSessionSave.hpp`
- `src/app/iggy3d/save/CurrentSessionSave.cpp`

Move from `Operations.*`:

- `writeProductCurrentSessionSave(...)`

Move from `Operations.cpp` private helpers:

- `recordProductSaveWriteResult(...)`

Remove `writeProductCurrentSessionSave(...)` from `Operations.hpp/.cpp`.

## Required Behavior Preservation

Preserve current save behavior exactly:

- no active session returns a `ProductSaveWriteResult` with:
  - `status = "product_save_session_missing"`
  - `reasonCode = "product_save_session_missing"`
  - `durableReason = "not_requested"`
  - producer-side save receipt fields recorded through the same status/reason/source/save id/session-saved fields
- save root still comes from `options.saveRoot`
- save id hint still uses empty string when `window.saveSession.activeProductSaveId == "none"`
- attempt token stays `"attempt_002"`
- request state still points at `activeSession->state()`
- authored room still points at `activeRoom(window).authoredRoom` only when `activeRoom(window).hasAuthoredRoom`
- save type stays `"manual"`
- saved-at timestamp still uses `productSaveTimestampNowUtc()`
- durable write still calls `writeProductSessionSaveDurably(request)`
- successful write with a non-empty record id still updates `window.saveSession.activeProductSaveId`

## Expected Source Edits

Likely touched production files:

- `CMakeLists.txt`
- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/save/CurrentSessionSave.hpp`
- `src/app/iggy3d/save/CurrentSessionSave.cpp`
- `src/app/iggy3d/save/Flow.cpp`
- focused compiler-reported direct users

Add `src/app/iggy3d/save/CurrentSessionSave.cpp` to the `iggy3d` library source list near other `src/app/iggy3d/save/*.cpp` entries.

## Include Policy

- Files that call `writeProductCurrentSessionSave(...)` should include `app/iggy3d/save/CurrentSessionSave.hpp`.
- Do not keep `Operations.hpp` includes solely for current-session save.
- The new `.cpp` should include complete types directly.

## Non-Scope

Do not move or reshape:

- `createCreativeBlankSession(...)`
- `frameCreativeStageCameraOnOrigin(...)`
- `clearProductGameplayLaunchState(...)`
- product new-world launch
- product session launch/bootstrap operations
- save/load session launch
- save-slot browser/delete/recover operations
- product world-template operations
- creative world operations

Do not change:

- save/load durable format
- save catalog behavior
- receipt keys/order/values
- active-room ownership
- renderer/window/projection behavior
- CMake test definitions

No staging, commit, push, or window launch.

## Required Greps

After implementation, run and report:

```sh
rg -n "writeProductCurrentSessionSave|recordProductSaveWriteResult" /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/save/CurrentSessionSave.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/save/CurrentSessionSave.cpp
```

Expected classification:

- `Operations.hpp` has no `writeProductCurrentSessionSave(...)` declaration.
- `Operations.cpp` has no current-session save helper definitions.
- New current-session save files own the declaration/definition for `writeProductCurrentSessionSave(...)` and the private recorder.

Also run and report direct include cleanup:

```sh
rg -n "#include \"app/iggy3d/Operations.hpp\"" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
```

Classify remaining `Operations.hpp` include users. Do not widen scope just to chase includes.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_starter_menu_action_tests product_window_input_frame_tests product_save_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_save_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- receipt golden output changes
- save durable output/identity carry-forward behavior changes
- missing-session save receipt behavior changes
- extraction forces creative blank-stage wrappers into the save file
- compile fallout expands beyond direct include repairs

## Completion Brief

When done, report:

- files changed
- exact APIs moved/added
- where `writeProductCurrentSessionSave(...)` now lives
- remaining `Operations.hpp` include users and why they remain
- required grep classifications
- receipt golden result
- focused build/CTest results
- diff/whitespace checks
- confirmation that creative blank-stage wrappers were not moved

## Completion Brief

- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/Operations.hpp`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/save/Flow.cpp`
  - `src/app/iggy3d/save/CurrentSessionSave.hpp`
  - `src/app/iggy3d/save/CurrentSessionSave.cpp`
  - this task card
- Exact APIs moved/added:
  - Removed from `Operations.hpp` and declared in `save/CurrentSessionSave.hpp`: `writeProductCurrentSessionSave(...)`.
  - Moved into `save/CurrentSessionSave.cpp`: `recordProductSaveWriteResult(...)` and `writeProductCurrentSessionSave(...)`.
  - Added `src/app/iggy3d/save/CurrentSessionSave.cpp` to the `iggy3d` library source list near other save sources.
- New location:
  - `writeProductCurrentSessionSave(...)` now lives in `src/app/iggy3d/save/CurrentSessionSave.cpp`, declared by `src/app/iggy3d/save/CurrentSessionSave.hpp`.
- Caller include repair:
  - `src/app/iggy3d/save/Flow.cpp` now includes `app/iggy3d/save/CurrentSessionSave.hpp` directly and no longer includes `app/iggy3d/Operations.hpp` for current-session save.
- Remaining `Operations.hpp` include users:
  - Direct Operations-owned API users remain: `src/app/iggy3d/creative/CreativeWorldOperations.cpp`, `src/app/iggy3d/world/ProductSessionLaunch.cpp`, and `src/app/iggy3d/world/ProductNewWorldLaunch.cpp` for creative blank-stage / gameplay-launch cleanup wrappers; `src/app/iggy3d/Operations.cpp` for its own declarations.
  - Existing non-E217 include users remain in `src/app/iggy3d/window/InputFrame.cpp` and several product tests; they do not call `writeProductCurrentSessionSave(...)` and were not chased because this card explicitly says not to widen into include cleanup.
- Required grep classifications:
  - `Operations.hpp`: no `writeProductCurrentSessionSave(...)` declaration remains.
  - `Operations.cpp`: no current-session save helper definitions remain.
  - `CurrentSessionSave.hpp/.cpp`: own the moved public declaration/definition and private recorder.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- Focused build/CTest results:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_starter_menu_action_tests product_window_input_frame_tests product_save_bridge_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_save_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 5/5.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card found no hits.
- Confirmations:
  - Creative blank-stage wrappers were not moved.
  - Product new-world launch, product session launch/bootstrap operations, save/load session launch, save-slot browser/delete/recover operations, product world-template operations, creative world operations, save/load durable format, save catalog behavior, receipt keys/order/values, CMake test definitions, staging, commit, push, and window launch were not changed.
