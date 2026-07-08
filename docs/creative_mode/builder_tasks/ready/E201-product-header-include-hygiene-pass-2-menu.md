# E201: Product Header Include Hygiene - Pass 2 Menu Headers

## Status

Ready.

## Context

E125 proved the include-hygiene pattern on `ReceiptBuilder.hpp` and
`window/RendererLifecycle.hpp`: headers that only mention
`ProductAppWindowState` by reference/pointer should forward-declare the type,
while `.cpp` files that read/write fields include the full
`ProductAppWindowState.hpp` directly.

Current live header census after E200 still shows menu headers pulling in the
full god-struct header even though their declarations only carry window state by
reference. This pass is intentionally small and mechanical.

## Objective

Remove the direct `ProductAppWindowState.hpp` include from two menu headers and
make concrete implementation/test users include their own dependency directly.

Target headers only:

- `src/app/iggy3d/menu/Transitions.hpp`
- `src/app/iggy3d/menu/ActionHandlers.hpp`

## Scope

Allowed edits:

- `src/app/iggy3d/menu/Transitions.hpp`
- `src/app/iggy3d/menu/Transitions.cpp`
- `src/app/iggy3d/menu/ActionHandlers.hpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- direct compile fallout in production/test `.cpp` or `.hpp` files that
  genuinely use `ProductAppWindowState` as a complete type and previously got
  it transitively through one of the two target headers;
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
   - `src/app/iggy3d/menu/Transitions.cpp`
   - `src/app/iggy3d/menu/ActionHandlers.cpp`
4. Build. For any fallout, add direct
   `#include "app/iggy3d/ProductAppWindowState.hpp"` to the file that actually
   instantiates, reads, writes, or otherwise needs the complete type.

## Escape Hatch

Stop and move this card to `blocked/` with evidence if:

- either target header needs a complete `ProductAppWindowState` type after all;
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

cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_menu_transitions_tests product_starter_menu_action_tests product_new_world_menu_action_tests product_window_input_frame_tests product_frontend_router_tests product_receipt_key_order_tests -j10

ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_menu_transitions_tests|product_starter_menu_action_tests|product_new_world_menu_action_tests|product_window_input_frame_tests|product_frontend_router_tests|product_receipt_key_order_tests)$' --output-on-failure

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
