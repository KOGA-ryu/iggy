# E202: Product Header Include Hygiene - Pass 3 Input Routing

## Status

Done.

## Context

E125 and E201 proved the include-hygiene pattern: headers that only mention
`ProductAppWindowState` by reference/pointer should forward-declare it, while
implementation files that read/write fields include the full
`ProductAppWindowState.hpp` directly.

Current live census still shows input-routing headers pulling in the full
window-state definition through declarations. This pass keeps the scope small
and mechanical.

## Objective

Remove the direct `ProductAppWindowState.hpp` include from three input-routing
headers and make concrete users include their own dependency directly when
needed.

Target headers only:

- `src/app/iggy3d/menu/InputRouter.hpp`
- `src/app/iggy3d/input/ControllerActionRouting.hpp`
- `src/app/iggy3d/input/InteractionModeState.hpp`

## Scope

Allowed edits:

- `src/app/iggy3d/menu/InputRouter.hpp`
- `src/app/iggy3d/menu/InputRouter.cpp`
- `src/app/iggy3d/input/ControllerActionRouting.hpp`
- `src/app/iggy3d/input/ControllerActionRouting.cpp`
- `src/app/iggy3d/input/InteractionModeState.hpp`
- `src/app/iggy3d/input/InteractionModeState.cpp`
- direct compile fallout in production/test `.cpp` or `.hpp` files that
  genuinely use `ProductAppWindowState` as a complete type and previously got
  it transitively through one of the three target headers;
- this task card.

Do not touch other include-hygiene candidate headers in this pass.

Do not change function signatures, function bodies, struct layouts, routing
policy, receipt fields, CMake, or receipt golden.

## Required Work

For each target header:

1. Confirm every `ProductAppWindowState` use is incomplete-type safe:
   `ProductAppWindowState&`, `const ProductAppWindowState&`, or
   `ProductAppWindowState*` in declarations/request structs only.
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

   Preserve existing namespace style if the header already has one.
3. Ensure the paired implementation file includes the full type directly:
   - `src/app/iggy3d/menu/InputRouter.cpp`
   - `src/app/iggy3d/input/ControllerActionRouting.cpp`
   - `src/app/iggy3d/input/InteractionModeState.cpp`
4. Build. For any fallout, add direct
   `#include "app/iggy3d/ProductAppWindowState.hpp"` to the file that actually
   instantiates, reads, writes, or otherwise needs the complete type.

## Escape Hatch

Stop and move this card to `blocked/` with evidence if:

- any target header needs a complete `ProductAppWindowState` type after all;
- fallout expands beyond a handful of obvious direct include repairs;
- fixing fallout requires declaration/signature/body changes;
- tests reveal behavior or receipt output changes.

Do not solve a cascade by reverting the target header changes unless you are
blocking the card and explaining the cascade.

## Required Checks

Run:

```sh
grep -rl 'app/iggy3d/ProductAppWindowState.hpp' \
  /Users/kogaryu/iggy3d/src \
  /Users/kogaryu/iggy3d/apps \
  /Users/kogaryu/iggy3d/tests | wc -l

cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_starter_menu_action_tests product_interaction_mode_state_tests product_controller_action_routing_tests product_automation_dispatch_tests product_receipt_key_order_tests -j10

ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_starter_menu_action_tests|product_interaction_mode_state_tests|product_controller_action_routing_tests|product_automation_dispatch_tests|product_receipt_key_order_tests)$' --output-on-failure

git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden

git -C /Users/kogaryu/iggy3d diff --check
```

Run the include-count command before and after the edit. The raw count may rise
if hidden transitive dependencies become explicit; report that honestly.

Also run a focused trailing-whitespace scan over touched files.

## Completion Brief Requirements

Report:

- target headers forward-declared;
- confirmation that each target header uses `ProductAppWindowState` only
  incompletely;
- `.cpp` partners that now include the full type directly;
- fallout files given direct includes, if any;
- before/after direct includer count for `ProductAppWindowState.hpp`;
- focused build/CTest results;
- receipt golden diff result;
- `git diff --check` and trailing-whitespace results;
- confirmation that no signatures, behavior, CMake, or receipt fields changed.

## Completion Brief

- Card moved to done: `/Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done/E202-product-header-include-hygiene-pass-3-input-routing.md`
- Files changed:
  - `src/app/iggy3d/menu/InputRouter.hpp`
  - `src/app/iggy3d/menu/InputRouter.cpp`
  - `src/app/iggy3d/input/ControllerActionRouting.hpp`
  - `src/app/iggy3d/input/ControllerActionRouting.cpp`
  - `src/app/iggy3d/input/InteractionModeState.hpp`
  - `src/app/iggy3d/input/InteractionModeState.cpp`
  - `src/app/iggy3d/automation/AutomationGameplay.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `tests/unit/product_interaction_mode_state_tests.cpp`
  - `tests/unit/product_controller_action_routing_tests.cpp`
  - `docs/creative_mode/builder_tasks/done/E202-product-header-include-hygiene-pass-3-input-routing.md`
- Target headers forward-declared:
  - `InputRouter.hpp` removed `#include "app/iggy3d/ProductAppWindowState.hpp"` and now forward-declares `struct ProductAppWindowState;`.
  - `ControllerActionRouting.hpp` removed `#include "app/iggy3d/ProductAppWindowState.hpp"` and now forward-declares `struct ProductAppWindowState;`.
  - `InteractionModeState.hpp` removed `#include "app/iggy3d/ProductAppWindowState.hpp"` and now forward-declares `struct ProductAppWindowState;`.
- Incomplete-type confirmation:
  - `InputRouter.hpp` uses `ProductAppWindowState` only as non-owning context members and function parameters by reference.
  - `ControllerActionRouting.hpp` uses `ProductAppWindowState` only as a non-owning function parameter by reference.
  - `InteractionModeState.hpp` uses `ProductAppWindowState` only as non-owning request members and function parameters by reference.
- `.cpp` partners with direct full-type includes:
  - `InputRouter.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` directly.
  - `ControllerActionRouting.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` directly.
  - `InteractionModeState.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` directly.
- Fallout files given direct includes:
  - `AutomationGameplay.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` because it reads/writes `context.window`.
  - `InputFrame.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` because it reads/writes `window`.
  - `product_controller_action_routing_tests.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` because it instantiates `ProductAppWindowState`.
  - `product_interaction_mode_state_tests.cpp` now includes direct fixture dependencies: `Options.hpp`, `ProductAppWindowState.hpp`, `SaveBridge.hpp`, and `DefaultWorldTemplate.hpp`.
- Direct includer count for `ProductAppWindowState.hpp`:
  - Before: 62
  - After: 66
  - Count rose because three header includes were removed and seven concrete users now include the full type directly.
- Focused build:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_starter_menu_action_tests product_interaction_mode_state_tests product_controller_action_routing_tests product_automation_dispatch_tests product_receipt_key_order_tests -j10` passed.
- Focused CTest:
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_starter_menu_action_tests|product_interaction_mode_state_tests|product_controller_action_routing_tests|product_automation_dispatch_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 6/6.
- Receipt golden diff:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card passed.
- Confirmations:
  - No function signatures changed.
  - No function bodies, struct layouts, behavior, routing policy, receipt fields, CMake, or receipt golden changed.
- Concerns/deferred:
  - None for this slice.
