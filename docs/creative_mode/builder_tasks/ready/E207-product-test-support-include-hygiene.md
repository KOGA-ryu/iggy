# E207: Product Test Support Include Hygiene

## Status

Ready.

## Context

E201-E206 moved production header include hygiene forward. After E206, the only
production headers still directly including `ProductAppWindowState.hpp` appear
to require a complete type by value:

- `src/app/iggy3d/AppKernel.hpp`
- `src/app/iggy3d/window/Loop.hpp`

Do not chase those in this pass.

The remaining obvious include-hygiene candidates are product test support
headers that only pass `ProductAppWindowState` by reference into existing
production helpers.

## Objective

Remove unnecessary direct `ProductAppWindowState.hpp` includes from the two
test support headers that do not instantiate or dereference the window state.

Target headers only:

- `tests/unit/ProductActiveSurfaceTestSupport.hpp`
- `tests/unit/ProductReceiptTestSupport.hpp`

## Scope

Allowed edits:

- `tests/unit/ProductActiveSurfaceTestSupport.hpp`
- `tests/unit/ProductReceiptTestSupport.hpp`
- direct compile fallout in test `.cpp` files that genuinely instantiate,
  read, write, or otherwise need a complete `ProductAppWindowState` and
  previously got it transitively through one of these support headers
- this task card

Do not edit production source in this pass.

Do not edit `tests/unit/ProductAsciiRoomWindowTestSupport.hpp`: it constructs a
`ProductAppWindowState` and writes fields, so it needs the complete type.

Do not change helper names, helper signatures, helper behavior, receipt fields,
CMake, or receipt golden.

## Required Work

1. Confirm:
   - `ProductActiveSurfaceTestSupport.hpp` uses `ProductAppWindowState` only as
     a non-owning reference passed to active-surface resolver helpers.
   - `ProductReceiptTestSupport.hpp` uses `ProductAppWindowState` only as
     `const ProductAppWindowState&` passed to `buildProductAppReceipt(...)`.
2. Remove the direct `#include "app/iggy3d/ProductAppWindowState.hpp"` from
   both target headers.
3. Add an explicit forward declaration in each target header:

   ```cpp
   namespace iggy3d {
   struct ProductAppWindowState;
   }
   ```

   Preserve the existing namespace style and keep the helper APIs under
   `namespace iggy3d::test`.
4. Build. For any fallout, add direct
   `#include "app/iggy3d/ProductAppWindowState.hpp"` to the test file that
   actually instantiates, reads, writes, or otherwise needs the complete type.

## Escape Hatch

Stop and move this card to `blocked/` with evidence if:

- either target support header needs a complete `ProductAppWindowState` type
  after all;
- fixing fallout requires helper signature/body changes;
- receipt helper behavior or active-surface helper behavior changes.

## Required Verification

Run:

```sh
rg -n 'ProductAppWindowState.hpp' /Users/kogaryu/iggy3d/tests/unit/ProductActiveSurfaceTestSupport.hpp /Users/kogaryu/iggy3d/tests/unit/ProductReceiptTestSupport.hpp
grep -rl 'app/iggy3d/ProductAppWindowState.hpp' /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests | wc -l
cmake --build /Users/kogaryu/iggy3d/build --target product_menu_transitions_tests product_starter_menu_action_tests product_window_input_frame_tests product_frontend_router_tests product_creative_ui_command_receipt_tests product_creative_ui_projection_receipt_tests product_creative_ui_frame_tests product_creative_ui_window_frame_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_menu_transitions_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_frontend_router_tests|product_creative_ui_command_receipt_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_frame_tests|product_creative_ui_window_frame_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files and this card.

## Completion Brief Requirements

Report:

- target test support headers forward-declared;
- incomplete-type confirmation for each target header;
- any fallout test files given direct full-type includes;
- direct includer count before and after;
- confirmation that `ProductAsciiRoomWindowTestSupport.hpp` was not changed;
- focused build/CTest results;
- receipt golden diff result;
- `diff --check` and whitespace-scan results;
- confirmation that no helper signatures, helper behavior, receipt fields,
  CMake, production source, or receipt golden changed.
