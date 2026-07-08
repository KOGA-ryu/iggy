# E211 — Operations Split G2: Save Slot Operations

## Status

Done.

## Context

E209 preflighted the `Operations.cpp` seam split. E210 extracted the creative
baked active-room refresh service. The next low-risk seam is the save-slot
browser/delete/recover cluster that still lives in `Operations.cpp` and is
called directly by menu, automation, input, and save-delete tests.

This is an extraction-only card. Preserve behavior, receipt fields, receipt
keys/order/values, status strings, and save/load formats.

## Objective

Create a save-owned operations seam for save-slot selection, deleted-save
browser, recover, delete-confirmation, and soft-delete flow APIs.

Suggested new files:

- `src/app/iggy3d/save/SaveSlotOperations.hpp`
- `src/app/iggy3d/save/SaveSlotOperations.cpp`

Add the new `.cpp` to the `iggy3d` library source list in `CMakeLists.txt` near
the other `src/app/iggy3d/save/*.cpp` sources.

## Move Scope

Move these public declarations out of `src/app/iggy3d/Operations.hpp` and into
the new save-slot header:

- `enum class ProductSaveFlowOperation`
- `struct ProductSaveFlowRequest`
- `struct ProductSaveFlowResult`
- `productSaveFlowOperationName(...)`
- `initializeSelectedProductSaveSlot(...)`
- `moveSelectedProductSaveSlot(...)`
- `selectProductSaveSlotById(...)`
- `scanDeletedProductSavesForOptions(...)`
- `recordDeletedProductSaveSlots(...)`
- `selectDeletedProductSaveSlotById(...)`
- `openDeletedProductSaveBrowser(...)`
- `executeProductSaveRecover(...)`
- `openProductSaveDeleteConfirmation(...)`
- `cancelProductSaveDeleteConfirmation(...)`
- `executeProductSaveSoftDelete(...)`

Move the private helpers used only by that cluster into the new `.cpp`:

- `firstSelectableSaveSlot(...)`
- `recordSelectedProductSaveSlot(...)`
- `recordProductSaveSlotAction(...)`
- `recordProductSaveFlowRequest(...)`
- `recordProductSaveFlowResult(...)`
- `recordSelectedDeletedProductSaveSlot(...)`
- `initializeSelectedDeletedProductSaveSlot(...)`

Handle `saveSlotById(...)` carefully. It is currently used by both the
save-slot cluster and `launchProductContinueSave(...)` in `Operations.cpp`.
Acceptable options:

- make a small shared public/internal helper in `SaveSlotOperations.hpp` only if
  the name and scope remain save-slot specific; or
- leave a narrow file-local copy in `Operations.cpp` for continue-save launch
  while moving the save-slot cluster copy into the new `.cpp`.

Do not widen E211 just to eliminate that one helper coupling.

## Explicit Non-Scope

Do not move:

- `writeProductCurrentSessionSave(...)`
- `recordProductSaveWriteResult(...)`
- `recordProductSaveLoadSelection(...)`
- `productWorldTemplateFromOptions(...)`
- `launchProductNewWorld(...)`
- `launchProductContinueSave(...)`
- `launchProductLoadSaveSelection(...)`
- package/session bootstrap helpers
- creative world launch/open/save helpers
- renderer/Vulkan/projection/window-loop code
- save/load durable file formats or parser/writer code
- `ProductAppWindowState` storage

`executeProductSaveSoftDelete(...)`, `executeProductSaveRecover(...)`, and
`scanDeletedProductSavesForOptions(...)` may still call
`productWorldTemplateFromOptions(options)` from their new `.cpp`. Keep that
coupling narrow; do not drag world-template launch code into this slice.

## Expected Callers To Update

Likely production callers that should include the new save-slot header instead
of relying on `Operations.hpp` for this API:

- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `src/app/iggy3d/automation/AutomationSaveBrowser.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `src/app/iggy3d/Operations.cpp` if it still calls
  `initializeSelectedProductSaveSlot(...)` or any shared save-slot helper

Likely focused tests:

- `tests/unit/product_save_delete_executor_tests.cpp`
- any compiler-reported product menu/input/save tests that directly use these
  APIs

Do not update unrelated includes unless the build proves they are required.

## Required Greps

After the move:

```sh
rg -n "ProductSaveFlowOperation|ProductSaveFlowRequest|ProductSaveFlowResult|productSaveFlowOperationName|initializeSelectedProductSaveSlot|moveSelectedProductSaveSlot|selectProductSaveSlotById|scanDeletedProductSavesForOptions|recordDeletedProductSaveSlots|selectDeletedProductSaveSlotById|openDeletedProductSaveBrowser|executeProductSaveRecover|openProductSaveDeleteConfirmation|cancelProductSaveDeleteConfirmation|executeProductSaveSoftDelete" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/save/SaveSlotOperations.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/save/SaveSlotOperations.cpp
```

Expected result:

- no declarations for the moved public APIs remain in `Operations.hpp`;
- no definitions for the moved public APIs remain in `Operations.cpp`;
- `Operations.cpp` may retain launch-path call sites and/or a narrow
  `saveSlotById(...)` helper if builder chooses the local-copy option;
- the new save-slot files contain the moved declarations/definitions.

Also run a focused caller grep and classify remaining hits:

```sh
rg -n "initializeSelectedProductSaveSlot|moveSelectedProductSaveSlot|selectProductSaveSlotById|scanDeletedProductSavesForOptions|recordDeletedProductSaveSlots|selectDeletedProductSaveSlotById|openDeletedProductSaveBrowser|executeProductSaveRecover|openProductSaveDeleteConfirmation|cancelProductSaveDeleteConfirmation|executeProductSaveSoftDelete" \
  /Users/kogaryu/iggy3d/src/app/iggy3d \
  /Users/kogaryu/iggy3d/tests/unit \
  --glob '*.cpp' --glob '*.hpp'
```

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_save_delete_executor_tests product_starter_menu_action_tests product_window_input_frame_tests product_automation_dispatch_tests product_save_catalog_tests product_save_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_save_delete_executor_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_automation_dispatch_tests|product_save_catalog_tests|product_save_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening if:

- the move forces package/session launch or current-session save/write logic into
  the new save-slot file;
- a circular include appears that cannot be fixed by moving includes to `.cpp`
  files or by a narrow helper decision;
- receipt golden output changes;
- any save/delete/recover status or reason string changes;
- `product_save_delete_executor_tests` reveals the live catalog refresh or
  selection reclamp behavior changed.

## Completion Brief Required

Report:

- new header/source paths;
- exact public APIs moved from `Operations.hpp`;
- exact private helpers moved or intentionally left/duplicated with rationale;
- remaining `Operations.cpp` hits for save-slot APIs and their classification;
- caller include updates;
- receipt golden result;
- focused build/CTest result;
- `git diff --check` and trailing-whitespace result;
- confirmation that current-session save/write, world launch/load, package
  bootstrap, save format, receipt keys, staging, commit, push, and window launch
  were not changed.

## Completion Brief

- Card moved to done: yes.
- New header/source paths:
  - `src/app/iggy3d/save/SaveSlotOperations.hpp`
  - `src/app/iggy3d/save/SaveSlotOperations.cpp`
- Public APIs moved from `Operations.hpp`:
  - `ProductSaveFlowOperation`
  - `ProductSaveFlowRequest`
  - `ProductSaveFlowResult`
  - `productSaveFlowOperationName(...)`
  - `initializeSelectedProductSaveSlot(...)`
  - `moveSelectedProductSaveSlot(...)`
  - `selectProductSaveSlotById(...)`
  - `scanDeletedProductSavesForOptions(...)`
  - `recordDeletedProductSaveSlots(...)`
  - `selectDeletedProductSaveSlotById(...)`
  - `openDeletedProductSaveBrowser(...)`
  - `executeProductSaveRecover(...)`
  - `openProductSaveDeleteConfirmation(...)`
  - `cancelProductSaveDeleteConfirmation(...)`
  - `executeProductSaveSoftDelete(...)`
- Private helpers moved to `SaveSlotOperations.cpp`:
  - `firstSelectableSaveSlot(...)`
  - `recordSelectedProductSaveSlot(...)`
  - `recordProductSaveSlotAction(...)`
  - `recordProductSaveFlowRequest(...)`
  - `recordProductSaveFlowResult(...)`
  - `recordSelectedDeletedProductSaveSlot(...)`
  - `initializeSelectedDeletedProductSaveSlot(...)`
- Helper intentionally duplicated/left:
  - `saveSlotById(...)` remains file-local in `Operations.cpp` for
    `launchProductContinueSave(...)`.
  - A file-local copy also exists in `SaveSlotOperations.cpp` for the moved
    save-slot cluster.
  - Rationale: this follows the local-copy option from the card and avoids
    exposing an extra helper or moving continue-save launch behavior.
- Remaining `Operations.cpp` hits for moved save-slot APIs:
  - `initializeSelectedProductSaveSlot(saves.slots, window)` remains in
    `launchProductLoadSaveSelection(...)` as a caller.
  - No declarations remain in `Operations.hpp`.
  - No moved API definitions remain in `Operations.cpp`.
- Caller include updates:
  - `src/app/iggy3d/menu/ActionHandlers.cpp` includes
    `app/iggy3d/save/SaveSlotOperations.hpp`.
  - `src/app/iggy3d/automation/AutomationSaveBrowser.cpp` includes
    `app/iggy3d/save/SaveSlotOperations.hpp` and no longer includes
    `Operations.hpp`.
  - `src/app/iggy3d/window/InputFrame.cpp` includes
    `app/iggy3d/save/SaveSlotOperations.hpp`.
  - `tests/unit/product_save_delete_executor_tests.cpp` includes
    `app/iggy3d/save/SaveSlotOperations.hpp` and no longer includes
    `Operations.hpp`.
  - `src/app/iggy3d/Operations.cpp` includes
    `app/iggy3d/save/SaveSlotOperations.hpp` for the remaining launch-load
    call site.
- CMake source-list update:
  - Added `src/app/iggy3d/save/SaveSlotOperations.cpp` near the other
    `src/app/iggy3d/save/*.cpp` sources.
- Required greps:
  - Moved API declaration/definition grep shows the public declarations and
    definitions in `SaveSlotOperations.hpp/.cpp`.
  - `Operations.hpp` has no hits.
  - `Operations.cpp` has only the launch-load caller hit described above.
  - Focused caller grep shows the expected production/test callers plus the new
    save-slot source/header definitions.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`
  - Result: empty.
- Focused build:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_save_delete_executor_tests product_starter_menu_action_tests product_window_input_frame_tests product_automation_dispatch_tests product_save_catalog_tests product_save_bridge_tests product_receipt_key_order_tests -j10`
  - Result: passed.
  - Note: build emitted an existing unused-variable warning in
    `tests/unit/product_starter_menu_action_tests.cpp`; this slice did not
    touch that file.
- Focused CTest:
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_save_delete_executor_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_automation_dispatch_tests|product_save_catalog_tests|product_save_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure`
  - Result: 7/7 passed.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Result: passed.
  - Focused trailing-whitespace scan over touched files and this card.
  - Result: clean.
- Confirmed unchanged:
  - Current-session save/write was not moved.
  - World launch/load and package bootstrap were not moved.
  - Save durable formats and parser/writer code were not changed.
  - Receipt keys/order/values were not changed.
  - No staging, commit, push, or window launch was performed.
