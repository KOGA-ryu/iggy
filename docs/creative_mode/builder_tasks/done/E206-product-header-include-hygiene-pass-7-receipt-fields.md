# E206: Product Header Include Hygiene - Pass 7 Receipt Fields

## Status

Done.

## Context

E125 and E201-E205 proved the include-hygiene pattern: headers that only
mention `ProductAppWindowState` by reference/pointer should forward-declare it,
while implementation files that read/write fields include the full
`ProductAppWindowState.hpp` directly.

After E205, the remaining obvious product header candidate is
`receipt/ReceiptFields.hpp`. It currently includes the full window-state header
even though its `ProductAppWindowState` uses are receipt-appender declarations
that take `const ProductAppWindowState&`.

Do not chase `AppKernel.hpp` or `window/Loop.hpp` in this pass: both own
`ProductAppWindowState` by value and are not incomplete-type safe.

## Objective

Remove the direct `ProductAppWindowState.hpp` include from
`ReceiptFields.hpp` and make concrete receipt implementation users include
their own dependency directly when needed.

Target header only:

- `src/app/iggy3d/receipt/ReceiptFields.hpp`

## Scope

Allowed edits:

- `src/app/iggy3d/receipt/ReceiptFields.hpp`
- receipt `.cpp` implementation files that read fields from
  `ProductAppWindowState`
- `src/app/iggy3d/ReceiptBuilder.cpp` if it was relying on the target header
  for the complete window-state type
- direct compile fallout in production/test `.cpp` or `.hpp` files that
  genuinely instantiate, read, write, or otherwise need a complete
  `ProductAppWindowState` and previously got it transitively through
  `ReceiptFields.hpp`
- this task card

Do not touch `AppKernel.hpp`, `window/Loop.hpp`, or other include-hygiene
candidate headers in this pass.

Do not change function signatures, function bodies, struct layouts, receipt
field keys/order/values, CMake, or receipt golden.

## Required Work

1. Confirm every `ProductAppWindowState` use in `ReceiptFields.hpp` is
   incomplete-type safe: `const ProductAppWindowState&` in declarations only.
2. Replace:

   ```cpp
   #include "app/iggy3d/ProductAppWindowState.hpp"
   ```

   with a forward declaration:

   ```cpp
   namespace iggy3d {
   struct ProductAppWindowState;
   }
   ```

   Preserve existing namespace style.
3. Ensure each receipt appender implementation that reads `window` fields
   includes `app/iggy3d/ProductAppWindowState.hpp` directly.
4. Build. For any fallout, add direct
   `#include "app/iggy3d/ProductAppWindowState.hpp"` to the file that actually
   instantiates, reads, writes, or otherwise needs the complete type.

## Escape Hatch

Stop and move this card to `blocked/` with evidence if:

- `ReceiptFields.hpp` needs a complete `ProductAppWindowState` type after all;
- fallout expands beyond obvious direct include repairs;
- fixing fallout requires declaration/signature/body changes;
- tests reveal receipt key/order/value drift.

## Required Verification

Run:

```sh
grep -rl 'app/iggy3d/ProductAppWindowState.hpp' /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests | wc -l
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests product_creative_ui_command_receipt_tests product_creative_ui_projection_receipt_tests product_creative_ui_frame_tests product_creative_ui_window_frame_tests product_vulkan_room_frame_tests product_movement_debug_hud_tests product_top_down_map_overlay_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_creative_ui_command_receipt_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_frame_tests|product_creative_ui_window_frame_tests|product_vulkan_room_frame_tests|product_movement_debug_hud_tests|product_top_down_map_overlay_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files and this card.

## Completion Brief Requirements

Report:

- target header forward-declared;
- incomplete-type confirmation;
- `.cpp` files given direct full-type includes;
- fallout files, if any;
- direct includer count before and after;
- focused build/CTest results;
- receipt golden diff result;
- `diff --check` and whitespace-scan results;
- confirmation that no signatures, function bodies, struct layouts, receipt
  fields, CMake, or receipt golden changed.

## Completion Brief

- Card moved to done:
  `/Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done/E206-product-header-include-hygiene-pass-7-receipt-fields.md`
- Target header forward-declared:
  - `src/app/iggy3d/receipt/ReceiptFields.hpp` no longer includes
    `app/iggy3d/ProductAppWindowState.hpp`.
  - It now forward-declares `ProductAppWindowState`.
  - It also forward-declares `ProductWorldTemplate`, which had been a hidden
    transitive declaration from `ProductAppWindowState.hpp` and is also used
    only by const reference in this header.
- Incomplete-type confirmation:
  - `ReceiptFields.hpp` uses `ProductAppWindowState` only as
    `const ProductAppWindowState&` in receipt appender declarations.
  - `ReceiptFields.hpp` uses `ProductWorldTemplate` only as
    `const ProductWorldTemplate&` in the tail appender declaration.
- `.cpp` files given direct full-type includes:
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `src/app/iggy3d/receipt/CreativePickWireframeFields.cpp`
  - `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp`
  - `src/app/iggy3d/receipt/CreativeUiFields.cpp`
  - `src/app/iggy3d/receipt/DebugHudFields.cpp`
  - `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp`
  - `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
  - `src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp`
  - `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`
  - `src/app/iggy3d/receipt/PhysicsReceiptRecording.cpp`
  - `src/app/iggy3d/receipt/SaveStateFields.cpp`
  - `src/app/iggy3d/receipt/StartupProbeFields.cpp`
  - `src/app/iggy3d/receipt/TailFields.cpp`
  - `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`
- Fallout files:
  - None outside the expected receipt implementation set and
    `ReceiptBuilder.cpp`.
  - The first build exposed only the missing `ProductWorldTemplate` forward
    declaration in the target header; no signature/body changes were needed.
- Direct includer count for `ProductAppWindowState.hpp`:
  - Before edit: 70
  - After edit: 83
  - The raw count rose because one transitive receipt header include was
    replaced with explicit full-type includes in concrete implementation files
    that dereference or mutate window state.
- Focused build result:
  - Passed:
    `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests product_creative_ui_command_receipt_tests product_creative_ui_projection_receipt_tests product_creative_ui_frame_tests product_creative_ui_window_frame_tests product_vulkan_room_frame_tests product_movement_debug_hud_tests product_top_down_map_overlay_tests -j10`
- Focused CTest result:
  - Passed:
    `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_creative_ui_command_receipt_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_frame_tests|product_creative_ui_window_frame_tests|product_vulkan_room_frame_tests|product_movement_debug_hud_tests|product_top_down_map_overlay_tests)$' --output-on-failure`
  - Result: 8/8 tests passed.
- Receipt golden diff result:
  - Empty diff:
    `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`
- Checks:
  - Passed: `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over touched files and this card found no
    matches.
- Scope confirmation:
  - No signatures, function bodies, struct layouts, receipt fields, CMake, or
    receipt golden files were changed.
- Concerns/deferred:
  - None.
